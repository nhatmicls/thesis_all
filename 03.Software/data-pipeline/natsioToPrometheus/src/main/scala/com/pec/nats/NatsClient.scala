package com.pec.nats

import io.nats.client.*
import io.nats.client.api.StorageType
import io.nats.client.api.StreamConfiguration
import io.nats.client.api.StreamInfo
import monix.execution.Cancelable
import monix.execution.Scheduler.Implicits.global
import monix.reactive.*

import java.io.IOException
import java.time.Duration
import scala.util.Failure
import scala.util.Success
import scala.util.Try

import java.security.KeyStore
import java.io.BufferedInputStream
import java.io.FileInputStream
import javax.net.ssl.KeyManagerFactory
import javax.net.ssl.KeyManager
import javax.net.ssl.SSLContext
import javax.net.ssl.TrustManager
import javax.net.ssl.TrustManagerFactory
import java.security.SecureRandom

import collection.JavaConverters.*
import java.security.GeneralSecurityException

class NatsClient(urls: Seq[String], creds_path: Seq[String]) {
  import ConnectionOps.*
  import SSL.*

  val ctx = SSL.createSSLContext
  // val theNKey = NKey.createUser(null);
  val options = new Options.Builder()
    .servers(urls.toArray)
    .sslContext(ctx)
    .authHandler(Nats.credentials(creds_path.head))
    // .authHandler(new AuthHandler() {
    //   def getID(): Array[Char] =
    //     Try(theNKey.getPublicKey()).getOrElse(Array())
    //   def sign(nonce: Array[Byte]): Array[Byte] =
    //     Try(theNKey.sign(nonce)).getOrElse(Array())
    //   def getJWT(): Array[Char] = Array()
    // })
    .build()

  val connection = Nats.connect(options)
  lazy val jetStream = connection.jetStream
  lazy val jsm = connection.jetStreamManagement

  /** See [[ConnectionOps.subscribeToStream]]
    */
  def subscribeToStream(
      subjects: Seq[String],
      queueNameOpt: Option[String] = None
  ): Observable[Message] =
    connection.subscribeToStream(subjects, queueNameOpt)

  /** See [[ConnectionOps.pushSubscribeToJStream]]
    */
  def pushSubscribeToJStream(
      subjects: Seq[String],
      autoAck: Boolean = false,
      streamOpt: Option[String] = None,
      durableOpt: Option[String] = None,
      queueNameOpt: Option[String] = None
  ): Observable[Message] =
    connection.pushSubscribeToJStream(
      subjects,
      autoAck,
      streamOpt,
      durableOpt,
      queueNameOpt,
      Some(jetStream)
    )
}

object SSL {
  val KEYSTORE_PATH =
    "/pecom/tls/keystore.jks"
  val TRUSTSTORE_PATH =
    "/pecom/tls/truststore.jks"
  val STORE_PASSWORD = "password"
  val KEY_PASSWORD = "password"
  val ALGORITHM = "SunX509"

  def loadKeystore(path: String): KeyStore = {
    val store = KeyStore.getInstance("JKS")
    val in = new BufferedInputStream(new FileInputStream(path))
    try store.load(in, STORE_PASSWORD.toCharArray)
    finally if (in != null) in.close
    store
  }

  def createTestKeyManagers: Array[KeyManager] = {
    val store = loadKeystore(KEYSTORE_PATH)
    val factory = KeyManagerFactory.getInstance(ALGORITHM)
    factory.init(store, KEY_PASSWORD.toCharArray)
    factory.getKeyManagers
  }

  def createTestTrustManagers: Array[TrustManager] = {
    val store = loadKeystore(TRUSTSTORE_PATH)
    val factory = TrustManagerFactory.getInstance(ALGORITHM)
    factory.init(store)
    factory.getTrustManagers
  }

  def createSSLContext: SSLContext = {
    val ctx = SSLContext.getInstance(Options.DEFAULT_SSL_PROTOCOL)
    ctx.init(createTestKeyManagers, createTestTrustManagers, new SecureRandom)
    ctx
  }
}

object ConnectionOps {
  // Extension functions for [[Connection]]
  extension (connection: Connection)
    /** Subscribe to a Natsio stream, exposed as an [[Observable]]
      *
      * @param subjects
      *   list of subjects
      * @param queueNameOpt
      *   name of the queue (optional)
      * @return
      *   an [[Observable[Message]] ]
      */
    def subscribeToStream(
        subjects: Seq[String],
        queueNameOpt: Option[String] = None
    ): Observable[Message] = {
      Observable.create(OverflowStrategy.Unbounded) { sub =>
        val dispatcher = connection.createDispatcher(sub.onNext(_))
        queueNameOpt match {
          case Some(queue) => subjects.foreach(dispatcher.subscribe(_, queue))
          case None        => subjects.foreach(dispatcher.subscribe(_))
        }

        Cancelable(() =>
          subjects.foreach(dispatcher.unsubscribe)
          connection.closeDispatcher(dispatcher)
        )
      }
    }

    /** Subscribe to a Jetstream Stream, exposed as an Observable
      *
      * @param subjects
      *   list of subjects
      * @param autoAck
      *   whether to let Natsio auto-acknowledge messages
      * @param streamOpt
      *   name of the stream, optional. If [[None]], Natsio will search the
      *   stream based on the subject name.
      * @param durablOpt
      *   the name of the durable subscriber (optional). If [[None]], the
      *   subscriber is not durable.
      * @param queueNameOpt
      *   the name of the queue (optional)
      * @param jetStreamOpt
      *   an instance of [[JetStream]] (optional). If [[None]], a new instance
      *   is created.
      * @return
      *   an [[Observable[Message]] ]
      */
    def pushSubscribeToJStream(
        subjects: Seq[String],
        autoAck: Boolean = false,
        streamOpt: Option[String] = None,
        durableOpt: Option[String] = None,
        queueNameOpt: Option[String] = None,
        jetStreamOpt: Option[JetStream] = None
    ): Observable[Message] = {
      val optionBuilder = PushSubscribeOptions.builder()
      durableOpt.foreach(optionBuilder.durable)
      streamOpt.foreach(optionBuilder.stream)
      val options = optionBuilder.build()
      Observable.create(OverflowStrategy.Unbounded) { sub =>
        val dispatcher = connection.createDispatcher()
        val jetStream = jetStreamOpt.getOrElse(connection.jetStream)
        val subscriptions = queueNameOpt match {
          case Some(queue) =>
            subjects.map(
              jetStream.subscribe(
                _,
                queue,
                dispatcher,
                sub.onNext(_),
                false,
                options
              )
            )
          case None =>
            subjects.map(
              jetStream.subscribe(
                _,
                dispatcher,
                sub.onNext(_),
                autoAck,
                options
              )
            )
        }

        Cancelable(() =>
          subscriptions.foreach(_.drain(Duration.ZERO))
          connection.closeDispatcher(dispatcher)
        )
      }
    }
}

object JetStreamManagementOps {
  // Extension functions for [[JetStreamManagement]]
  extension (jsm: JetStreamManagement)
    /** Create a new JetStream stream. If it doesn't yet exist, a new one is
      * created. If it already exists, the list of [[Subjects]] will be updated.
      * [[storageType]] is ignored if the stream already exists.
      *
      * @param streamName
      *   the name of the new stream
      * @param storageType
      *   the type of storage, in memory or on disk
      * @param subjects
      *   the list of subjects belonging to this stream
      * @return
      *   a [[Try[StreamInfo]] ] of the new stream
      */
    def createOrUpdateStream(
        streamName: String,
        storageType: StorageType,
        subjects: Seq[String]
    ): Try[StreamInfo] = {
      val conf = StreamConfiguration
        .builder()
        .name(streamName)
        .storageType(storageType)
        .subjects(subjects: _*)
        .build()

      createOrUpdateStream(conf)
    }

    /** Create a new JetStream stream. If it doesn't yet exist, a new one is
      * created. If it already exists, the list of [[Subjects]] will be updated.
      * everything else in @param streamConf will be ignored.
      *
      * @param streamConf
      *   a [[StreamConfiguration]]
      * @return
      *   a [[Try[StreamInfo]] ] of the new stream
      */
    def createOrUpdateStream(
        streamConf: StreamConfiguration
    ): Try[StreamInfo] = {
      Try(jsm.getStreamInfo(streamConf.getName)) match {
        case f @ Failure(_) =>
          createStream(streamConf)
        case success @ Success(streamInfo) =>
          val oldStreamConf = streamInfo.getConfiguration
          val oldSubjects = oldStreamConf.getSubjects
          val newSubjects = streamConf.getSubjects

          val diffSubjects =
            newSubjects.asScala.toSet -- oldSubjects.asScala.toSet
          val combinedSubjects =
            newSubjects.asScala.toSet ++ oldSubjects.asScala.toSet

          if (diffSubjects.nonEmpty) {
            val updatedConfig = StreamConfiguration
              .builder(oldStreamConf)
              .subjects(combinedSubjects.toSeq: _*)
              .build()

            Try(jsm.updateStream(updatedConfig))
          } else success
      }
    }

    /** Check if a stream with a given [[streamName]] exists.
      *
      * @param streamName
      *   name of the Stream
      * @return
      *   the result.
      */
    def streamExists(streamName: String): Try[Boolean] =
      Try(jsm.getStreamInfo(streamName)) match {
        case Failure(e: JetStreamApiException) =>
          if (e.getErrorCode == 404)
            Try(false)
          else Failure(e)
        case Failure(e) => Failure(e)
        case Success(_) => Try(true)
      }

    /** Create a new stream.
      *
      * @param streamName
      *   name of the Stream
      * @param storageType
      *   the storage type of the new stream, in memory or on disk
      * @param subjects
      *   list of subjects belonging to this stream
      * @return
      *   a [[StreamInfo]] of the new stream
      */
    def createStream(
        streamName: String,
        storageType: StorageType,
        subjects: Seq[String]
    ): Try[StreamInfo] = {
      val streamConf = StreamConfiguration
        .builder()
        .name(streamName)
        .storageType(storageType)
        .subjects(subjects: _*)
        .build()

      createStream(streamConf)
    }

    /** Create a new stream, wrapper of [[JetStreamManagement#addStream]]
      *
      * @param streamConf
      *   configuration of the new stream
      */
    def createStream(streamConf: StreamConfiguration): Try[StreamInfo] =
      Try(jsm.addStream(streamConf))
}
