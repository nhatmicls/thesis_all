package com.pec.natsio2prom

import com.pec.natsioschema.schema.Metric.MetricValue
import com.pec.natsioschema.schema.Metric.MetricValue.*
import com.pec.natsioschema.schema.*
import prometheus.remote.WriteRequest
import prometheus.types.Label
import prometheus.types.Sample
import prometheus.types.TimeSeries

import language.postfixOps

/** Functions used to transform [[MetricSubmission]] received from Natsio to
  * Prometheus' remote write format.
  */
object Transformer {

  /** Make [[WriteRequest]] for Prometheus' remote write api.
    *
    * @param metricSubmissions
    *   a [[Seq]] of [[MetricSubmission]]
    * @return
    *   a [[WriteRequest]] to be used for Prometheus remote write
    */
  def makeWriteRequest(metricSubmissions: Seq[MetricSubmission]): WriteRequest =
    WriteRequest().update(
      _.timeseries :++= makeTimeSeries(metricSubmissions).map(_._2)
    )

  /** Turn a sequence of [[MetricSubmission]] into a sequence of [[TimeSeries]].
    * Special logic is used to merge samples belong to the same [[TimeSeries]]
    * into one [[TimeSeries]]. This is used for batch processing.
    *
    * @param metricSubmissions
    *   a sequence of [[MetricSubmission]]
    * @return
    *   a sequence of pairs [[Set[Label]] and [[TimeSeries]], where the time
    *   series have labels given by the [[Set[Label]] ]
    */
  def makeTimeSeries(
      metricSubmissions: Seq[MetricSubmission]
  ): Seq[(Set[Label], TimeSeries)] = {
    val samples =
      for {
        metricSubmission <- metricSubmissions
        metricGroup <- metricSubmission.metricGroups
        siteInfo <- metricSubmission.siteInfo.toSeq
        deviceInfo <- metricGroup.deviceInfo.toSeq
        metric <- metricGroup.metrics
        labels = makeLabels(siteInfo, deviceInfo, metric)
        sample <- extractSample(metricGroup.timestamp, metric)
      } yield (
        labels,
        sample
      )

    // Group all TimeSeries with the same label set
    samples
      .groupBy(_._1) // Group all TimeSeries with the same label set
      .map((labels, labelsAndSamples) =>
        (labels, labelsAndSamples.map(_._2))
      )
      .map((labels, ss) =>
        (
          labels,
          TimeSeries().update(
            _.labels :++= labels,
            _.samples :++= ss.sortBy(_.timestamp)
          )
        )
      )
      .toSeq
  }

  /** Make [[Label]] to be used in Prometheus' [[TimeSeries]] out of
    * [[SiteInfo]], [[DeviceInfo]], and [[Metric]].
    */
  def makeLabels(
      siteInfo: SiteInfo,
      deviceInfo: DeviceInfo,
      metric: Metric
  ): Set[Label] = {
    Set(
      Label("__name__", makeName(metric)),
      Label("tenant", siteInfo.tenant),
      Label("location", siteInfo.location),
      Label("manufacturer", deviceInfo.manufacturer),
      Label("model", deviceInfo.model),
      Label("device_SN", deviceInfo.serialNumber),
      Label("device_id", deviceInfo.deviceId),
      Label("driver_ver", deviceInfo.driverVersion)
    )
  }

  def extractSample(timeStamp: Long, metric: Metric): Option[Sample] = {
    metric.metricValue match {
      case Int32Value(v) =>
        Some(Sample(value = v, timestamp = timeStamp))
      case Uint32Value(v) =>
        Some(Sample(value = v, timestamp = timeStamp))
      case Int64Value(v) =>
        Some(Sample(value = v.toDouble, timestamp = timeStamp))
      case Uint64Value(v) =>
        Some(Sample(value = v.toDouble, timestamp = timeStamp))
      case FloatValue(v) =>
        Some(Sample(value = v, timestamp = timeStamp))
      case Enum16Value(v) =>
        Some(Sample(value = v, timestamp = timeStamp))
      case Enum32Value(v) =>
        Some(Sample(value = v, timestamp = timeStamp))
      case Bitfield16Value(v) =>
        Some(Sample(value = v, timestamp = timeStamp))
      case Bitfield32Value(v) =>
        Some(Sample(value = v, timestamp = timeStamp))
      case _ => None
    }
  }

  def makeName(metric: Metric): String = {
    s"${metric.name}_${metric.unit}"
  }
}
