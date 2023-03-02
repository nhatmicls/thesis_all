package com.pec.natsio2prom

import io.vertx
import io.vertx.core.Vertx
import io.vertx.core.buffer.Buffer
import io.vertx.ext.web.client.HttpRequest
import io.vertx.ext.web.client.HttpResponse
import io.vertx.ext.web.client.WebClient
import monix.eval.Task

import scala.util.Failure
import scala.util.Success

import concurrent.duration.*

object VertxMonix {
  extension (req: HttpRequest[Buffer])
    def sendBytes(bytes: Array[Byte]): Task[HttpResponse[Buffer]] = {
      Task.cancelable0 { (scheduler, cb) =>
        val cancelable = scheduler.scheduleOnce(0.second) {
          req
            .sendBuffer(Buffer.buffer(bytes))
            .onSuccess(res => cb(Success(res)))
            .onFailure(t => cb(Failure(t)))
        }

        Task {
          cancelable.cancel()
        }
      }
    }
}
