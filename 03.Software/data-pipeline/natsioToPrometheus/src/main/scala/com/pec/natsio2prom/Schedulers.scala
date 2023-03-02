package com.pec.natsio2prom

object Schedulers {
    val ioScheduler = monix.execution.Scheduler.io("io-thread", false)
}
