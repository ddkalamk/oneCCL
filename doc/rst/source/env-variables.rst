=====================
Environment Variables
=====================

Collective algorithms selection
###############################

CCL_<coll_name>
***************
**Syntax**

To set a specific algorithm for the whole message size range:

::

  CCL_<coll_name>=<algo_name>

To set a specific algorithm for a specific message size range:

::

  CCL_<coll_name>="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

- ``<coll_name>`` is selected from a list of available collective operations (`Available collectives`_).
- ``<algo_name>`` is selected from a list of available algorithms for a specific collective operation (`Available algorithms`_).
- ``<size_range>`` is described by the left and the right size borders in a format ``<left>-<right>``. 
  Size is specified in bytes. Use reserved word ``max`` to specify the maximum message size.

|product_short| internally fills algorithm selection table with sensible defaults. User input complements the selection table. 
To see the actual table values set ``CCL_LOG_LEVEL=info``.

.. rubric:: Example

:: 

  CCL_ALLREDUCE="recursive_doubling:0-8192;rabenseifner:8193-1048576;ring:1048577-max"

Available collectives
*********************

Available collective operations (``<coll_name>``):

-   ``ALLGATHERV``
-   ``ALLREDUCE``
-   ``ALLTOALL``
-   ``ALLTOALLV``
-   ``BARRIER``
-   ``BCAST``
-   ``REDUCE``
-   ``REDUCE_SCATTER``
-   ``SPARSE_ALLREDUCE``


Available algorithms
********************

Available algorithms for each collective operation (``<algo_name>``):

``ALLGATHERV`` algorithms
+++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left
   
   * - ``direct``
     - Based on ``MPI_Iallgatherv``
   * - ``naive``
     - Send to all, receive from all
   * - ``flat``
     - Alltoall-based algorithm
   * - ``multi_bcast``
     - Series of broadcast operations with different root ranks


``ALLREDUCE`` algorithms
++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on ``MPI_Iallreduce``
   * - ``rabenseifner``
     - Rabenseifner’s algorithm
   * - ``starlike``
     - May be beneficial for imbalanced workloads
   * - ``ring`` 
     - reduce_scatter + allgather ring.
       Use ``CCL_RS_CHUNK_COUNT`` and ``CCL_RS_MIN_CHUNK_SIZE``
       to control pipelining on reduce_scatter phase.
   * - ``ring_rma``
     - reduce_scatter+allgather ring using RMA communications
   * - ``double_tree``
     - Double-tree algorithm
   * - ``recursive_doubling``
     - Recursive doubling algorithm
   * - ``2d``
     - Two-dimensional algorithm (reduce_scatter + allreduce + allgather)


``ALLTOALL`` algorithms
++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on ``MPI_Ialltoall``
   * - ``naive``
     - Send to all, receive from all


``ALLTOALLV`` algorithms
++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on ``MPI_Ialltoallv``
   * - ``naive``
     - Send to all, receive from all


``BARRIER`` algorithms
++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left
   
   * - ``direct``
     - Based on ``MPI_Ibarrier``
   * - ``ring``
     - Ring-based algorithm


``BCAST`` algorithms
++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on ``MPI_Ibcast``
   * - ``ring`` 
     - Ring
   * - ``double_tree``
     - Double-tree algorithm
   * - ``naive``
     - Send to all from root rank


``REDUCE`` algorithms
+++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on ``MPI_Ireduce``
   * - ``rabenseifner``
     - Rabenseifner’s algorithm
   * - ``tree``
     - Tree algorithm
   * - ``double_tree``
     - Double-tree algorithm


``REDUCE_SCATTER`` algorithms
+++++++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on ``MPI_Ireduce_scatter_block``
   * - ``ring`` 
     - Use ``CCL_RS_CHUNK_COUNT`` and ``CCL_RS_MIN_CHUNK_SIZE``
       to control pipelining.


``SPARSE_ALLREDUCE`` algorithms
+++++++++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``ring``
     - Ring-allreduce based algorithm
   * - ``mask``
     - Mask matrix based algorithm
   * - ``allgatherv``
     - 3-allgatherv based algorithm

.. note::
    WARNING: ``ccl::sparse_allreduce`` is experimental and subject to change.


CCL_RS_CHUNK_COUNT
++++++++++++++++++
**Syntax**

:: 

  CCL_RS_CHUNK_COUNT=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``COUNT``
     - Maximum number of chunks.

**Description**

Set this environment variable to specify maximum number of chunks for reduce_scatter phase in ring allreduce.


CCL_RS_MIN_CHUNK_SIZE
+++++++++++++++++++++
**Syntax**

:: 

  CCL_RS_MIN_CHUNK_SIZE=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``SIZE``
     - Minimum number of bytes in chunk.

**Description**

Set this environment variable to specify minimum number of bytes in chunk for reduce_scatter phase in ring allreduce. Affects actual value of ``CCL_RS_CHUNK_COUNT``.


Fusion
######

CCL_FUSION
**********

**Syntax**

:: 

  CCL_FUSION=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``1``
     - Enable fusion of collective operations
   * - ``0``
     - Disable fusion of collective operations (**default**)

**Description**

Set this environment variable to control fusion of collective operations.
The real fusion depends on additional settings described below.


CCL_FUSION_BYTES_THRESHOLD
**************************
**Syntax**

:: 

  CCL_FUSION_BYTES_THRESHOLD=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``SIZE``
     - Bytes threshold for a collective operation. If the size of a communication buffer in bytes is less than or equal
       to ``SIZE``, then |product_short| fuses this operation with the other ones.

**Description**

Set this environment variable to specify the threshold of the number of bytes for a collective operation to be fused.


CCL_FUSION_COUNT_THRESHOLD
**************************
**Syntax**

:: 

  CCL_FUSION_COUNT_THRESHOLD=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``COUNT``
     - The threshold for the number of collective operations.
       |product_short| can fuse together no more than ``COUNT`` operations at a time.

**Description**

Set this environment variable to specify count threshold for a collective operation to be fused.


CCL_FUSION_CYCLE_MS
*******************
**Syntax**

:: 

  CCL_FUSION_CYCLE_MS=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``MS``
     - The frequency of checking for collectives operations to be fused, in milliseconds:
       
       - Small ``MS`` value can improve latency. 
       - Large ``MS`` value can help to fuse larger number of operations at a time.

**Description**

Set this environment variable to specify the frequency of checking for collectives operations to be fused.


CCL_ATL_TRANSPORT
#################
**Syntax**

:: 

  CCL_ATL_TRANSPORT=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``mpi``
     - MPI transport (**default**).
   * - ``ofi``
     - OFI (Libfabric\*) transport.

**Description**

Set this environment variable to select the transport for inter-node communications.

CCL_UNORDERED_COLL
##################
**Syntax**

:: 

  CCL_UNORDERED_COLL=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``1``
     - Enable execution of unordered collectives.
       You have to additionally specify ``match_id``.
   * - ``0``
     - Disable execution of unordered collectives (**default**).

**Description**

Set this environment variable to enable execution of unordered collective operations on different nodes. 


CCL_PRIORITY
############
**Syntax**

:: 

  CCL_PRIORITY=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``direct``
     - You have to explicitly specify priority using ``priority``.
   * - ``lifo``
     - Priority is implicitly increased on each collective call. You do not have to specify priority.
   * - ``none``
     - Disable prioritization (**default**).

**Description**

Set this environment variable to control priority mode of collective operations. 


CCL_WORKER_COUNT
################
**Syntax**

:: 

  CCL_WORKER_COUNT=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``N``
     - The number of worker threads for |product_short| rank (``1`` if not specified).

**Description**

Set this environment variable to specify the number of |product_short| worker threads.


CCL_WORKER_AFFINITY
###################
**Syntax**

:: 

  CCL_WORKER_AFFINITY=<proclist>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <proclist> 
     - Description
   * - ``auto``
     - Workers are automatically pinned to last cores of pin domain.
       Pin domain depends from process launcher.
       If ``mpirun`` from |product_short| package is used then pin domain is MPI process pin domain.
       Otherwise, pin domain is all cores on the node.
   * - ``n1,n2,..``
     - Affinity is explicitly specified for all local workers.

**Description**

Set this environment variable to specify cpu affinity for |product_short| worker threads.


CCL_LOG_LEVEL
#############
**Syntax**

:: 

  CCL_LOG_LEVEL=<value>

**Arguments**

.. list-table:: 
   :header-rows: 1
   :align: left
   
   * - <value> 
   * - ``error``
   * - ``warn`` (**default**)
   * - ``info``
   * - ``debug``
   * - ``trace``

**Description**

Set this environment variable to control logging level.


CCL_MAX_SHORT_SIZE
##################
**Syntax**

:: 

  CCL_MAX_SHORT_SIZE=<value>

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``SIZE``
     - Bytes threshold for a collective operation (``0`` if not specified). If the size of a communication buffer in bytes is less than or equal to ``SIZE``, then |product_short| does not split operation between workers. Applicable for ``allreduce``, ``reduce`` and ``broadcast``.

**Description**

Set this environment variable to specify the threshold of the number of bytes for a collective operation to be split.
