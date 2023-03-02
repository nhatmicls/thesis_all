package com.pec

import com.pec.nats.NatsClient
import com.pec.natsio2prom.RemoteWrite
import com.pec.natsio2prom.Transformer
import com.pec.natsio2prom.config.Config
import com.pec.natsioschema.schema.MetricSubmission
import io.nats.client.JetStream
import io.nats.client.Message
import io.vertx.core.buffer.Buffer
import io.vertx.ext.web.client.HttpResponse
import monix.eval.Task
import monix.execution.Cancelable
import monix.execution.Scheduler.Implicits.global
import monix.reactive.Observable
import monix.reactive.observables.ConnectableObservable
import org.xerial.snappy.Snappy
import prometheus.remote.WriteRequest
import prometheus.types.Label
import prometheus.types.Sample
import prometheus.types.TimeSeries

object Pipe {

  /** Natsio to Prometheus remote write
    *
    * @param ob
    *   an [[Observable]] consisting of Natsio's [[Message]]
    * @param rw
    *   a [[RemoteWrite]] instance (to push to Prometheus)
    * @param batchStrategy
    *   how to batch messages up to reduce the number of calls to remote write
    *   endpoints
    *
    * @return
    *   an [[Observable[Either[Throwable,Seq[Message]] ]]]. The [[Seq[Message]]
    *   ] part consists of all messages that have been processed. Acks to Natsio
    *   are sent for messages that have successfully been processed.
    */
  def fromNatsioStream(
      ob: Observable[Message],
      rw: RemoteWrite,
      batchStrategy: Observable[Message] => Observable[Seq[Message]]
  ): Observable[Seq[Message]] =
    batchStrategy(ob)
      .mapEval { msgs =>
        val writeRequest = Transformer.makeWriteRequest(
          msgs.map(msg =>
            MetricSubmission.parseFrom(Snappy.uncompress(msg.getData))
          )
        )

        rw.push(writeRequest).map { _ =>
          msgs.foreach(_.ack)
          msgs
        }
      }

  /** Natsio to Prometheus remote write. Auto-ack should be used in the Natsio
    * subscription
    *
    * @param ob
    *   an [[Observable]] consisting of Natsio's [[Message]]
    * @param rw
    *   a [[RemoteWrite]] instance (to push to Prometheus)
    * @param parallelism
    *   number of parallel writes allowed
    * @param batchStrategy
    *   how to batch messages up to reduce the number of calls to remote write
    *   endpoints
    *
    * @return
    *   an [[Observable[Buffer]] ] consisting response from Server
    */
  def fromNatsioStreamParallel(
      ob: Observable[Message],
      rw: RemoteWrite,
      parallelism: Int,
      batchStrategy: Observable[Message] => Observable[Seq[Message]]
  ): Observable[HttpResponse[Buffer]] = {
    val tsOb = // Convert to TimeSeries
      batchStrategy(ob).map { msgs =>
        Transformer.makeTimeSeries(
          msgs.map(msg =>
            MetricSubmission.parseFrom(Snappy.uncompress(msg.getData))
          )
        )
      }

    tsOb.flatMap { seq =>
      val shards = seq
        .groupBy(_._1.hashCode % parallelism)
        .values
        .map(_.map(_._2))
        .map(ts => WriteRequest().update(_.timeseries :++= ts))

      Observable
        .from(shards)
        .mapParallelUnordered(parallelism)(writeRequest =>
          rw.push(writeRequest)
        )
    }
  }
}
