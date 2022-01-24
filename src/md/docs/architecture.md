# Architecture

This document describes Reign's design in terms of:

* Its approach to data durability and data consistency
* Its data model
* The architecture of the individual trading system
* The systems place in the overall platform architecture
* The systems approach to recovering from failure

## State & Durability

In terms of durability, there are three types of state:
```kroki-blockdiag
blockdiag {
    group {
        color = "green"
        "Transactional"
    }
    group {
        color = "blue"
        "Implicit"
    }
    group {
        color = "red"
        "Transient"
    }
}
```

* **Transactional State**
    This is state that is stored in a transactional datastore. The durability of this data depends on both the durability of the datastore and how transactionally it is updated. For example, in a typical RDBMS, as long as updates are made within a transaction and the SDBMS is run in a durable fasion, then this state will likely be durable.
    In general, *Transactional State* tends to be considered durable.

* **Implicit State**
    This is state which is inferred from the observable state of third party systems and inherits the durability of the third party system. In the case of system failure, the state is lost, but can be recovered by inferring it from the external system. Therefore, this can generally be considered durable.


* **Transient State**
    This is any in-memory state. This state is not durable and in the case of failure, data loss will occur. The data loss can be minimised through the use of disk persistence and backups, however, some data loss should be expected.


The rule of thumb is that *transient* state will be lost on failure, *transactional* state is not and *implicit* state can be recovered.

The fewer durability and consistency guarantees that are provided, the better a system will perform, but the higher the liklihood of data loss. By carefully managing which state can be lost, which state must never be lost and which state can be recovered from external sources, we can design a system that is observably durable, consistent and safe, without giving up on performance and scalability.

Reign's data model is designed to be carefully split into these three types of state to achieve the goals of a safe and fast trading platform.

## Data Model

## System

## Platform

## Failure Recovery
