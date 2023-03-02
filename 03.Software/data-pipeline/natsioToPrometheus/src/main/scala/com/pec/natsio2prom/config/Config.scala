package com.pec.natsio2prom.config

import scala.concurrent.duration.*

trait Config {
  lazy val remoteWriteEndpoints: Seq[String]
  lazy val hostAWSEndpoints: Seq[String]
  lazy val natsioEndpoints: Seq[String]
  lazy val natsioTopics: Seq[String]
  lazy val natsioSubscribeId: Option[String]
  lazy val credsPATH: Seq[String]
  lazy val batchSize: Option[Int]
  lazy val pushShardCount: Option[Int]
}
