lazy val myOrg = "com.so"
lazy val scalaVer = "3.1.0"

lazy val circeVer = "0.14.1"
lazy val jnatsVer = "2.13.0"
lazy val snappyJavaVer = "1.1.8.4"
lazy val monixVer = "3.4.0"
lazy val vertxWebClientVer = "4.1.4"

lazy val circe = "io.circe" %% "circe-parser" % circeVer
lazy val jnats = "io.nats" % "jnats" % jnatsVer
lazy val snappyJava = "org.xerial.snappy" % "snappy-java" % snappyJavaVer
lazy val monix = "io.monix" %% "monix" % monixVer
lazy val vertxWebClient = "io.vertx" % "vertx-web-client" % vertxWebClientVer

lazy val scalapbDep =
  "com.thesamet.scalapb" %% "scalapb-runtime" % scalapb.compiler.Version.scalapbVersion % "protobuf"

lazy val dockerBaseImageVal = "adoptopenjdk/openjdk16:debianslim-jre"

lazy val commonSettings = Seq(
  scalaVersion := scalaVer,
  organization := myOrg,
  compileOrder := CompileOrder.JavaThenScala
)

lazy val dockerSettings = Seq(
  dockerBaseImage := dockerBaseImageVal,
  dockerRepository := Some("afreisberg")
)

lazy val root = (project in file("."))
  .settings(commonSettings)
  .dependsOn(prometheusRemoteWrite, natsioToPrometheus)

lazy val prometheusRemoteWrite = (project in file("prometheusRemoteWrite"))
  .settings(commonSettings)
  .settings(
    name := "prometheus_remote_write",
    version := "0.1-SNAPSHOT",
    Compile / PB.targets := Seq(
      scalapb.gen() -> (Compile / sourceManaged).value / "scalapb"
    ),
    libraryDependencies ++= Seq(
      scalapbDep
    )
  )

lazy val natsioSchema = (project in file("natsioSchema"))
  .settings(commonSettings)
  .settings(
    name := "natsio_schema",
    version := "0.1-SNAPSHOT",
    Compile / PB.targets := Seq(
      scalapb.gen() -> (Compile / sourceManaged).value / "scalapb"
    ),
    libraryDependencies ++= Seq(
      scalapbDep
    )
  )

lazy val natsioToPrometheus = (project in file("natsioToPrometheus"))
  .settings(commonSettings)
  .enablePlugins(JavaAppPackaging, DockerPlugin)
  .settings(dockerSettings)
  .settings(
    Docker / packageName := "natsio-to-prometheus",
    Docker / version := "0.1-SNAPSHOT"
  )
  .settings(
    name := "natsio_to_Prometheus",
    version := "0.1-SNAPSHOT",
    libraryDependencies ++= Seq(
      jnats,
      snappyJava,
      monix,
      vertxWebClient
    )
  )
  .dependsOn(prometheusRemoteWrite, natsioSchema)
  .aggregate(prometheusRemoteWrite, natsioSchema)

lazy val playground = (project in file("playground"))
  .settings(commonSettings)
  .settings(
    name := "playground",
    version := "0.1-SNAPSHOT"
  )
  .dependsOn(natsioToPrometheus)
