package com.pec

import com.pec.nats.NatsClient
import com.pec.natsio2prom.RemoteWrite
import com.pec.natsio2prom.Transformer.*
import com.pec.natsio2prom.config.Config
import com.pec.natsio2prom.config.EnvConfig
import com.pec.natsioschema.schema.Metric.*
import com.pec.natsioschema.schema.*
import io.nats.client.Nats
import io.nats.client.api.StorageType
import io.vertx.core.Vertx
import io.vertx.core.buffer.Buffer
import io.vertx.ext.web.client.HttpResponse
import io.vertx.ext.web.client.WebClient
import monix.eval.Task
import monix.execution.Scheduler
import monix.execution.Scheduler.Implicits.global
import monix.reactive.Observable
import org.xerial.snappy.Snappy
import prometheus.remote.*
import prometheus.types.*

import scala.concurrent.duration.*
import scala.language.postfixOps

object Test extends App {
  // testNatsio(EnvConfig)
  // testProtobuf()
  submitMetric(EnvConfig)

  // testRemoteWrite(Seq(fakeMetric)).runAsync {
  //   case Left(t) =>
  //     println(t.getMessage)
  //   case Right(res) =>
  //     println("Successfull!")
  // }

  // Observable.intervalAtFixedRate(3.second).mapEval(_ => testRemoteWrite()).subscribe()

  // Thread.sleep(100000000)

  def submitMetric(config: Config): Unit = {
    import com.pec.nats.ConnectionOps.*
    import com.pec.nats.JetStreamManagementOps.*
    val natsClient = new NatsClient(config.natsioEndpoints, config.credsPATH)

    Observable
      .interval((0.001).second)
      .foreach(_ =>
        val metric = fakeMetric
        val compressed = Snappy.compress(metric.toByteArray)
        natsClient.jetStream
          .publish("inverter-metrics.v1.test.local.micls", compressed)
      )

    // natsClient
    //   .pushSubscribeToJStream(Seq("inverter-metrics.v1.test.local.micls"))
    //   .foreach(msg =>
    //     println(
    //       s"${msg.getSubject} ${MetricSubmission.parseFrom(Snappy.uncompress(msg.getData))}"
    //     )
    //   )
  }

  def fakeMetric: MetricSubmission =
    MetricSubmission().update(
      _.siteInfo.tenant := "PECOM-TEST",
      _.siteInfo.location := "HCMC-TEST",
      _.metricGroups :++= Seq(
        MetricGroup().update(
          _.timestamp := System.currentTimeMillis,
          _.deviceInfo.model := "SHP 75",
          _.deviceInfo.manufacturer := "SMA",
          _.deviceInfo.serialNumber := "A2009272557",
          _.deviceInfo.driverVersion := "1.0.0",
          _.deviceInfo.deviceId := "1",
          _.metrics :++= Seq(
            Metric().update(
              _.name := "PPVphAB",
              _.unit := "V",
              _.int32Value := (Math.random * 10).toInt
            ),
            Metric().update(
              _.name := "PPVphA",
              _.unit := "A",
              _.floatValue := Math.random.toFloat * 10
            ),
            Metric().update(
              _.name := "PPVphX",
              _.unit := "A",
              _.floatValue := Math.random.toFloat * 10
            ),
            Metric().update(
              _.name := "PPVphY",
              _.unit := "A",
              _.floatValue := Math.random.toFloat * 10
            ),
            Metric().update(
              _.name := "PPVphZ",
              _.unit := "A",
              _.floatValue := Math.random.toFloat * 10
            ),
            Metric().update(
              _.name := "PPVphD",
              _.unit := "A",
              _.floatValue := Math.random.toFloat * 10
            ),
            Metric().update(
              _.name := "PPVphE",
              _.unit := "A",
              _.floatValue := Math.random.toFloat * 10
            )
          )
        )
      )
    )

  def testRemoteWrite(
      metricSubmissions: Seq[MetricSubmission]
  ): Task[HttpResponse[Buffer]] = {
    lazy val vertx = Vertx.vertx()
    lazy val webClient = WebClient.create(vertx)

    val simulate = metricSubmissions.map(s =>
      MetricSubmission.parseFrom(
        s.toByteArray
      )
    )
    val writeRequest = makeWriteRequest(simulate)

    println(simulate)
    println(writeRequest)

    val remoteWrite =
      RemoteWrite(
        EnvConfig.remoteWriteEndpoints,
        EnvConfig.hostAWSEndpoints,
        webClient
      )
    remoteWrite.push(writeRequest)
  }

  def testSchema(): Unit = {
    val metricSubmission = fakeMetric

    val metricValues = for {
      metricGroup <- metricSubmission.metricGroups
      metric <- metricGroup.metrics
    } yield metric.metricValue

    metricValues.map { (value: MetricValue) =>
      import MetricValue.*
      value match {
        case Int32Value(intValue)   => println(s"int21Value ${intValue}")
        case FloatValue(floatValue) => println(s"floatValue ${floatValue}")
        case _                      => println("none")
      }
    }

    println("---------------")
    println(metricSubmission)
    println(makeWriteRequest(Seq(metricSubmission)))
  }

  def testProtobuf(): Unit = {
    val wr = WriteRequest(timeseries =
      Seq(
        TimeSeries(
          labels = Seq(Label("__name__", "PECOM3")),
          samples = Seq(Sample(1234, System.currentTimeMillis()))
        )
      )
    )

    val writeRequest = WriteRequest()
    writeRequest.update(
      _.timeseries :++= Seq(
        TimeSeries().update(
          _.labels :++= Seq(Label("__name__", "PECOM3")),
          _.samples :++= Seq(Sample(123, System.currentTimeMillis()))
        )
      )
    )

    println(writeRequest.toByteArray)
  }

  def testNatsio(config: Config): Unit = {
    import com.pec.nats.ConnectionOps.*
    import com.pec.nats.JetStreamManagementOps.*
    val natsClient = new NatsClient(config.natsioEndpoints, config.credsPATH)

    println(
      natsClient.jsm
        .createOrUpdateStream("test-stream", StorageType.Memory, Seq("test.>"))
    )

    natsClient
      .pushSubscribeToJStream(Seq("test.*.very"))
      .foreach(msg =>
      // println(s"${msg.getSubject} ${msg.getData.map(_.toChar).mkString}")
      )

    // natsClient
    //   .subscribeToStream(Seq("test.*.very"))
    //   .foreach(msg =>
    //     println(s"${msg.getSubject} ${msg.getData.map(_.toChar).mkString}")
    //   )

    Observable
      .interval((0.001).second)
      .doOnNext(_ =>
        println(
          natsClient.jetStream
            .publish("test.cool.very", "test".getBytes)
            .getStream
        )
        Task.unit
      )
      .onErrorHandle(println)
      .subscribe()

    // Observable
    //   .interval(1.second)
    //   .foreach(_ =>
    //     natsClient.connection
    //       .request("test.cool.very", "test".getBytes)
    //       .thenAccept(a =>
    //         println(s"${a.getSubject} ${a.getData.map(_.toChar).mkString}")
    //       )
    //   )

    // Observable
    //   .interval(1.second)
    //   .foreach(_ =>
    //     natsClient.connection.publish("test.bad.very", "test".getBytes)
    //   )

    // val ob = natsClient.connection.subscribeToStream(
    //   Seq("*")
    // )
    // ob.foreach(sub => println(sub.getSubject))
    // ob.bufferTimedAndCounted(3 seconds, 10)
    //   .map(_.map(_.getSubject))
    //   .foreach(println)

    // Observable.interval(1 second).foreach(_ => natsClient.connection.publish("mysubject", "abc".getBytes))

    // natsClient.connection.publish("mysubject", "abc".getBytes)
  }
}
