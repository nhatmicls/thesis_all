package com.pec.natsio2prom.config

import java.net.{URL, URI}
import scala.concurrent.duration.Duration
import scala.util.Try

object EnvConfig extends Config {
  override lazy val remoteWriteEndpoints: Seq[String] = getStrings(
    "RemoteWriteEndpoints"
  )
  override lazy val hostAWSEndpoints: Seq[String] = getStrings(
    "HostAWSEndpoints"
  )
  override lazy val natsioEndpoints: Seq[String] = getStrings("NatsioEndpoints")
  override lazy val natsioTopics: Seq[String] = getStrings("NatsioTopics")
  override lazy val natsioSubscribeId: Option[String] =
    Try(getString("NatsioSubscribeId")).toOption

  override lazy val credsPATH: Seq[String] = getStrings(
    "credsPATH"
  )
  override lazy val batchSize: Option[Int] = getString("BatchSize").toIntOption
  override lazy val pushShardCount: Option[Int] = getString(
    "PushShardCount"
  ).toIntOption

  private def getStrings(envKey: String): Seq[String] =
    getString(envKey).split(' ').toIndexedSeq

  private def getString(envKey: String): String =
    System.getenv(envKey)
}
