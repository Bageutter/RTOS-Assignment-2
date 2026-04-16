# Thread Pipeline Flow

The following Mermaid diagram shows the execution flow for the three-thread pipeline in `assignment2_template.c`.

```mermaid
flowchart LR
    %% Main function and setup
    A[Main Function] --> B[Initialize Data / Create Pipe A to B / Create Shared Memory / Init Semaphores]
    B --> C[Create Thread A]
    C --> D[Create Thread B]
    D --> E[Create Thread C]
    E --> F[Wait for all threads to finish]
    F --> G[Cleanup semaphores / Exit]

    %% Thread A flow
    subgraph A1["Thread A"]
        direction TB
        A1S[Start] --> A1W[sem_wait sem_A]
        A1W --> A1O[Open input file]
        A1O --> A1Q{Read line from file?}
        A1Q -->|Yes| A1P[Write line to pipe A to B]
        A1P --> A1B[sem_post sem_B]
        A1B --> A1W
        A1Q -->|No| A1C[Close pipe write end]
        A1C --> A1E[sem_post sem_B EOF]
        A1E --> A1X[End]
    end

    %% Thread B flow
    subgraph B1["Thread B"]
        direction TB
        B1S[Start] --> B1W[sem_wait sem_B]
        B1W --> B1R[Read from pipe A to B]
        B1R --> B1Q{Data received?}
        B1Q -->|Yes| B1M[Write to shared memory]
        B1M --> B1C[sem_post sem_C]
        B1C --> B1W
        B1Q -->|No| B1Z[Clear shared memory / EOF signal]
        B1Z --> B1E[sem_post sem_C]
        B1E --> B1X[End]
    end

    %% Thread C flow
    subgraph C1["Thread C"]
        direction TB
        C1S[Start] --> C1W[sem_wait sem_C]
        C1W --> C1R[Read from shared memory]
        C1R --> C1Q{EOF signal? / Empty memory after first read}
        C1Q -->|Yes| C1X[Exit]
        C1Q -->|No| C1H{Contains end_header?}
        C1H -->|Yes| C1K[Set headerFinished = true / Skip writing]
        C1H -->|No| C1D{headerFinished?}
        C1D -->|Yes| C1O[Write line to output file / Increment counter]
        C1D -->|No| C1S2[Skip header line]
        C1K --> C1N[Clear shared memory]
        C1O --> C1N
        C1S2 --> C1N
        C1N --> C1A[sem_post sem_A]
        C1A --> C1W
    end

    %% Branch connections
    C --> A1S
    D --> B1S
    E --> C1S

    %% Data flow hints
    A1P -.->|A->B data| B1R
    B1M -.->|B->C data| C1R
```
