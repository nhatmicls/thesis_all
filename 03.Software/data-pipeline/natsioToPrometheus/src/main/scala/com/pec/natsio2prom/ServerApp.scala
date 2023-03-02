package com.pec.natsio2prom

import com.pec.Pipe
import com.pec.nats.ConnectionOps.*
import com.pec.nats.JetStreamManagementOps.*
import com.pec.nats.NatsClient
import com.pec.natsio2prom.config.EnvConfig
import com.pec.natsioschema.schema.MetricSubmission
import io.nats.client.api.StorageType
import io.nats.client.api.StreamConfiguration
import monix.eval.Task
import monix.execution.Scheduler.Implicits.global
import org.xerial.snappy.Snappy

import java.time.Duration
import scala.util.Failure
import scala.util.Success
import io.vertx.ext.web.client.WebClient
import io.vertx.core.Vertx

object ServerApp extends App {
  lazy val config = EnvConfig

  lazy val streamName = "inverter-metrics"
  lazy val subjects = config.natsioTopics

  lazy val vertx = Vertx.vertx()
  lazy val webClient = WebClient.create(vertx)
  lazy val natsioClient = NatsClient(config.natsioEndpoints, config.credsPATH)
  lazy val remoteWrite =
    RemoteWrite(
      config.remoteWriteEndpoints,
      config.hostAWSEndpoints,
      webClient
    )

  lazy val streamConf = StreamConfiguration
    .builder()
    .name(streamName)
    .subjects(subjects: _*)
    .storageType(StorageType.File)
    .maxAge(Duration.ofHours(5))
    .build()

  natsioClient.jsm.createOrUpdateStream(streamConf) match {
    case Failure(t)          => throw t
    case Success(streamInfo) => println(streamInfo)
  }

  val metricOb = natsioClient.pushSubscribeToJStream(
    subjects,
    autoAck = true,
    streamOpt = Some(streamName),
    durableOpt = config.natsioSubscribeId
  )

  Pipe
    .fromNatsioStreamParallel(
      metricOb,
      remoteWrite,
      parallelism = config.pushShardCount.getOrElse(4),
      _.bufferTumbling(config.batchSize.getOrElse(20))
    )
    .doOnNext(e => Task(println(e.bodyAsString())))
    .onErrorHandle(println)
    .subscribe()

  println("Server started.")
}
