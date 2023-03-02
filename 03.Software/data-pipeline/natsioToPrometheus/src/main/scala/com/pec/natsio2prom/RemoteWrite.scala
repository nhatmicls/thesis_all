package com.pec.natsio2prom

import io.vertx
import io.vertx.core.Vertx
import io.vertx.core.buffer.Buffer
import io.vertx.ext.web.client.HttpRequest
import io.vertx.ext.web.client.HttpResponse
import io.vertx.ext.web.client.WebClient
import io.vertx.core.http.HttpMethod
import monix.eval.Task
import org.xerial.snappy.Snappy
import prometheus.remote.WriteRequest
import java.sql.Timestamp
import java.text.SimpleDateFormat
import java.util.Date

import Schedulers.ioScheduler
import VertxMonix.*

class RemoteWrite(
    remoteWriteEndpoints: Seq[String],
    hostEndpoints: Seq[String],
    webClient: WebClient
) {
  // TODO: should somehow use all of them here?
  lazy val endpoint = remoteWriteEndpoints.head
  lazy val hostendpoint = hostEndpoints.head

  /** Push data to Prometheus remote write endpoint
    *
    * @param writeRequest
    *   a [[WriteRequest]] defined by Prometheus proto format
    *
    * @return
    *   response from server, a [[Task[HttpResponse[Buffer]] ]]
    */
  def push(writeRequest: WriteRequest): Task[HttpResponse[Buffer]] = {
    val body = compress(writeRequest)
    val stf = new SimpleDateFormat("yyyyMMdd'T'HHmmss'Z'")
    val utc_timestamp = stf.format(new Timestamp(System.currentTimeMillis()))

    webClient
      .postAbs(endpoint)
      .putHeader("Host", hostendpoint)
      .putHeader("X-Amz-Date", utc_timestamp)
      .putHeader("User-Agent", "Prometheus/2.20.1")
      .putHeader("X-Prometheus-Remote-Write-Version", "0.1.0")
      .putHeader("Content-type", "application/x-protobuf")
      .putHeader("Content-Encoding", "snappy")
      .sendBytes(body)
      .executeOn(ioScheduler)
  }

  def compress(writeRequest: WriteRequest): Array[Byte] =
    Snappy.compress(writeRequest.toByteArray)
}
