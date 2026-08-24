# MsoMgmt HAL Documentation

## Version History

| Date | Comment | Version |
| --- | --- | --- |
| 2024-06-10 | Initial release. MSO HAL header migration to GitHub (RDKB-52500), refined against MTA HAL review comments. | 1.0.0 |
| 2026-08-24 | Specification rebuilt against `include/mso_mgmt_hal.h`. A previously documented initialization and teardown lifecycle, its context handle type, and a device-status call were removed: this interface has never declared any of them. Canonical topic set completed. | Unreleased |

Four version identities apply to this repository and are deliberately kept apart, because conflating them misrepresents the interface:

- **Release tag** \- `1.0.0`, the only tag in this repository, and the release the table above describes.
- **Document revision** \- the rows of the table above. The `Unreleased` row is this documentation change, which has not been cut into a release.
- **Generated-site version string** \- `git describe --tags` currently yields `1.0.0-3-gbf6a9a4`. That is the `1.0.0` tag plus three commits, not a version. `docs/generate_docs.sh` passes it verbatim as `PROJECT_VERSION`, so it appears in the generated documentation without being a release identity.
- **Interface version** \- **none exists.** This header defines no version macro, so a caller cannot query the interface version at compile time or at runtime. See `Variability Management`.

*Derived from `CHANGELOG.md` (single `1.0.0` section, which carries no per-release date), the `1.0.0` tag's commit date, and `docs/generate_docs.sh`.*

## Acronyms

- `HAL` \- Hardware Abstraction Layer
- `RDK-B` \- Reference Design Kit for Broadband Devices
- `MSO` \- Multiple System Operator
- `PoD` \- Password of the Day
- `API` \- Application Programming Interface
- `ABI` \- Application Binary Interface
- `OEM` \- Original Equipment Manufacturer
- `DOCSIS` \- Data Over Cable Service Interface Specification
- `SNMP` \- Simple Network Management Protocol
- `OID` \- Object Identifier
- `SLA` \- Service Level Agreement

*Bounded to the terms this document uses.*

## Description

The diagram below describes the high-level software architecture of the MsoMgmt HAL module stack.

```mermaid
flowchart TD;
    RDKBStack[RDK-B Stack] <--> CcspPandMSsp[CcspPandMSsp];
    CcspPandMSsp <--> MsoHal["MSO Management HAL (libhal_msomgmt.so)"];
    MsoHal <--> VendorSoftware[Vendor Software];
```

The MSO Management HAL is the interface through which RDK-B middleware validates an operator's **password of the day** and manages the **PoD seed** from which that password is derived. The `OEM` or third-party vendor supplies the implementation behind the interface; RDK-B supplies the caller. The owning middleware service is `CcspPandMSsp`.

The declared contract is exactly three functions: one password validation call and two seed accessors. They are listed by identifier under `API Surface`.

**What this interface does not cover.** The header's file-level description frames the deployment context as broadband `DOCSIS` devices in `MSO` environments (`include/mso_mgmt_hal.h:22-26`) and then lists device provisioning and configuration, data-model read and write access, event notification and security management (`:28-32`). **No declaration in this header implements any of those.** There is no provisioning call, no data-model accessor, no notification surface and no security primitive beyond password validation. A caller must take the three declarations as the whole of the contract and must not plan against the broader wording.

*Derived from `include/mso_mgmt_hal.h:22-36, 187, 202, 217` for the interface, and the superproject `README.md:97` for the owning service.*

## Optional Components

The following are optional and at the vendor's discretion.

- `rdkbEncryptedClientSeed` — `mso_get_pod_seed` is documented as retrieving the decrypted seed "from the configuration file or the `rdkbEncryptedClientSeed` `SNMP` `OID`" (`include/mso_mgmt_hal.h:207`). Either source satisfies the contract; the interface does not select between them, and a caller cannot determine from the return value which one was used.
- On-demand seed decryption — `mso_set_pod_seed` records that "Newer Broadband Devices MUST decrypt the seed on demand when this function is called" (`:200`). Whether decryption happens on demand is therefore a device-class-dependent implementation behavior rather than a property of every conforming implementation.

The interface declares **no optional functions**. All three declarations (`:187`, `:202`, `:217`) are unconditional, with no build-variability guard around any of them, so a conforming implementation exposes all three on every product.

*Derived from `include/mso_mgmt_hal.h:200, 207` and the unconditional declarations at `:187`, `:202`, `:217`.*

## Component Runtime Execution Requirements

### Initialization and Startup

**This interface has no initialization or de-initialization entry point and no context handle.** A caller invokes any of the three functions directly, in any order, without opening or closing a session. That is stated affirmatively here because its absence is easy to mistake for an undocumented lifecycle: there is no call to make first, and nothing to release afterwards.

The MsoMgmt HAL client module does not have explicit dependencies on other APIs.

The three functions available to a caller are:

- `mso_validatepwd()`
- `mso_set_pod_seed()`
- `mso_get_pod_seed()`

Third-party vendors must implement this HAL according to their system's specific requirements.

The interface specifies no startup ordering, no readiness handshake and no behavior for a call made before the vendor implementation is ready. A caller must not assume that any of the three functions is safe to call at an arbitrary point in the boot sequence, and must not assume that it is unsafe either — this interface does not establish it.

*Derived from `include/mso_mgmt_hal.h:187, 202, 217`, and from the dependency and vendor-implementation statements carried by the previous revision of this page.*

### Threading Model

**There is no requirement to make this interface thread safe.** Any module that invokes the API must ensure calls are made in a thread-safe manner. This differs deliberately from HALs that declare themselves thread safe; the obligation to synchronize rests with the caller.

Vendors may implement internal threading and event mechanisms to meet their operational requirements. These mechanisms must be designed to ensure thread safety when interacting with the HAL interface. Proper cleanup of allocated resources (memory, file handles, threads) is mandatory when the vendor software terminates or closes its connection to the HAL.

*Derived from the threading statement carried by the previous revision of this page, which is this repository's own policy statement.*

### Process Model

All APIs are expected to be called from multiple processes. Due to this concurrent access, vendors must implement protection mechanisms within their API implementations to handle multiple processes calling the same API simultaneously. This is crucial to ensure data integrity, prevent race conditions, and maintain the overall stability and reliability of the system.

*Derived from the process-model statement carried by the previous revision of this page.*

### Memory Model

Every buffer this interface touches is supplied by the caller. No declared function returns a pointer, so the HAL hands back no storage for a caller to free and there is no ownership transfer in either direction.

#### Caller Responsibilities

- Manage memory passed to specific functions as outlined in the API documentation, including allocation and deallocation, to prevent leaks.
- **All three parameters are caller-allocated.** `pwd` is "pre-allocated by the caller" (`include/mso_mgmt_hal.h:174`); the `pSeed` argument of `mso_set_pod_seed` is a "pre-allocated buffer" (`:194`); the `pSeed` argument of `mso_get_pod_seed` is a "pre-allocated buffer" written by the callee (`:209`).
- **Both seed buffers must be at least 64 bytes** (`:194`, `:209`).
- After `mso_get_pod_seed` returns, the caller's buffer holds a **decrypted secret**, and "for security reasons, the `pSeed` buffer MUST be manually overwritten after use" (`:215`).
- **The interface does not specify whether these buffers are NUL-terminated or fixed-length**, and does not state how a caller determines the length of a seed it has just retrieved. A caller must not assume either representation.

#### Module Responsibilities

- Handle and deallocate memory used for internal operations.
- Release all internally allocated memory upon closure to prevent leaks.
- The interface does not state whether an implementation may retain a caller-supplied pointer beyond the duration of the call, so a caller cannot rely on the buffer becoming private again on return.

**No memory footprint limit is specified for this interface.** Neither the header nor any other artifact in this repository states a maximum resident size for an implementation, so none is asserted here.

*Derived from `include/mso_mgmt_hal.h:174, 194, 209, 215`, and from the caller/module split carried by the previous revision of this page.*

### Power Management Requirements

The HAL is not involved in any of the power management operations.

*Derived from the power-management statement carried by the previous revision of this page.*

### Asynchronous Notification Model

There are no asynchronous notifications. The header corroborates this: it declares no callback typedef and no registration function, so there is no notification surface for a caller to subscribe to and no delivery thread to reason about.

*Derived from `include/mso_mgmt_hal.h` (no callback typedef declared) and the notification statement carried by the previous revision of this page.*

### Blocking calls

**Synchronous and Responsive:** All APIs within this module should operate synchronously and complete within a reasonable timeframe based on the complexity of the operation.

**Timeout Handling:** To ensure resilience in cases of unresponsiveness, vendors should implement appropriate timeouts within their implementations where failure due to lack of response is a possibility.

**Blocking behavior is not specified.** This interface does not specify whether `mso_validatepwd`, `mso_set_pod_seed` or `mso_get_pod_seed` may block; a caller must not assume either behavior. This is recorded as unspecified rather than resolved because the previous revision of this page asserted both positions — under its implementer note, that the interface "is designed to block execution if the necessary hardware is not yet ready", and under this topic, that "it is imperative that they do not block or suspend execution of the main thread" — and nothing in the header establishes either. A caller that must not block should treat these calls as potentially blocking and isolate them accordingly.

**No per-API response-time budget or timeout value is specified for this interface.** No maximum response time, no default timeout and no configuration option for one is stated by the header or by any other artifact in this repository.

*Derived from `include/mso_mgmt_hal.h:187, 202, 217` (which state no blocking or timing behavior) and from the contradictory statements carried by the previous revision of this page.*

### Internal Error Handling

**Synchronous Error Handling:** All APIs must return errors synchronously as a return value. This ensures immediate notification of errors to the caller.

**Internal Error Reporting:** The HAL is responsible for reporting any internal system errors, such as out-of-memory conditions, through the return value.

**Focus on Logging for Errors:** For system errors, the HAL should prioritize logging the error details for further investigation and resolution.

The interface provides exactly two error domains and no others: `mso_pwd_ret_status` for `mso_validatepwd`, and the `RETURN_OK` / `RETURN_ERR` integer pair for both seed accessors. There is no error-detail accessor, no `errno` convention and no way for a caller to distinguish the failure causes the header groups behind a single `RETURN_ERR` — `mso_set_pod_seed` documents "invalid seed, decryption error" (`include/mso_mgmt_hal.h:198`) and `mso_get_pod_seed` documents "retrieval error, decryption error" (`:213`) against that one value.

*Derived from `include/mso_mgmt_hal.h:100, 104, 127-135, 198, 213` and the error-handling statements carried by the previous revision of this page.*

### Persistence Model

There is no requirement for the HAL to persist any setting information.

Note that `mso_get_pod_seed` is documented as retrieving the seed from the configuration file or the `rdkbEncryptedClientSeed` `SNMP` `OID` (`include/mso_mgmt_hal.h:207`), so a seed does live somewhere on the device. Where it is stored, and whether a seed written by `mso_set_pod_seed` survives a restart, is a vendor implementation matter this interface does not specify.

*Derived from `include/mso_mgmt_hal.h:207` and the persistence statement carried by the previous revision of this page.*

## Non functional requirements

The following non functional requirements should be supported by the component.

### Logging and debugging requirements

The component is required to record all errors and critical informative messages to aid in identifying, debugging, and understanding the functional flow of the system. Logging should be implemented using the syslog method, as it provides robust logging capabilities suited for system-level software. The use of `printf` is discouraged unless `syslog` is not available.

All HAL components must adhere to a consistent logging process. When logging is necessary, it should be performed into the `mso_vendor_hal.log` file, which is located in either the `/var/tmp/` or `/rdklogs/logs/` directories.

Logs must be categorized according to the following log levels, as defined by the Linux standard logging system, listed here in descending order of severity:

- **FATAL:** Critical conditions, typically indicating system crashes or severe failures that require immediate attention.
- **ERROR:** Non-fatal error conditions that nonetheless significantly impede normal operation.
- **WARNING:** Potentially harmful situations that do not yet represent errors.
- **NOTICE:** Important but not error-level events.
- **INFO:** General informational messages that highlight system operations.
- **DEBUG:** Detailed information typically useful only when diagnosing problems.
- **TRACE:** Very fine-grained logging to trace the internal flow of the system.

Each log entry should include a timestamp, the log level, and a message describing the event or condition. This standard format will facilitate easier parsing and analysis of log files across different vendors and components.

This interface does not state whether seed or password values may appear in log output. Given the secret handling `mso_get_pod_seed` documents (`include/mso_mgmt_hal.h:215`), a caller should treat their absence from the vendor log as unguaranteed rather than assured.

*Derived from the logging requirements carried by the previous revision of this page, and from `include/mso_mgmt_hal.h:215`.*

### Memory and performance requirements

**Client Module Responsibility:** The client module using the HAL is responsible for allocating and deallocating memory for any data structures required by the HAL's APIs. This includes buffers used to receive data from the HAL — for this interface, the seed buffer described under `Memory Model`.

**Vendor Implementation Responsibility:** Third-party vendors, when implementing the HAL, may allocate memory internally for their specific operational needs. It is the vendor's sole responsibility to manage and deallocate this internally allocated memory.

Neither a memory footprint limit nor a performance budget is specified for this interface; see `Memory Model` and `Blocking calls` respectively.

*Derived from the memory and performance statements carried by the previous revision of this page.*

### Quality Control

To ensure the highest quality and reliability, it is strongly recommended that third-party quality assurance tools such as `Coverity`, `Black Duck` and `Valgrind` be employed to thoroughly analyze the implementation. The goal is to detect and resolve potential issues such as memory leaks, memory corruption, or other defects before deployment.

Furthermore, both the HAL wrapper and any third-party software interacting with it must prioritize robust memory management practices. This includes meticulous allocation, deallocation, and error handling to guarantee a stable and leak-free operation. The decrypted seed `mso_get_pod_seed` writes into a caller buffer (`include/mso_mgmt_hal.h:215`) makes this more than a hygiene matter: a leaked or un-overwritten buffer leaks a credential.

**Keeping this document accurate.** Every topic above and below names the file its content was derived from. **Any change to a file this document cites obliges a review of the topics that cite it** — in practice, any change to `include/mso_mgmt_hal.h` obliges a review of `Optional Components`, `Memory Model`, `Internal Error Handling`, `Theory of operation and key concepts`, `Data Structures and Defines`, `API Surface`, `Sequence Diagram` and `State Diagram`. The responsible reviewer is this repository's code owner, `@rdkcentral/rdkb-hal-advisory` (`.github/CODEOWNERS`).

*Derived from the quality-control statements carried by the previous revision of this page, `.github/CODEOWNERS`, and `include/mso_mgmt_hal.h:215`.*

### Licensing

The implementation is expected to be released under the Apache License 2.0, which is the license the interface header itself carries (`include/mso_mgmt_hal.h:1-18`).

*Derived from `include/mso_mgmt_hal.h:1-18` and the licensing statement carried by the previous revision of this page.*

### Build Requirements

The source code should be capable of, but not be limited to, building under the Yocto distribution environment. The recipe should deliver a shared library named `libhal_msomgmt.so`.

*Derived from the build statement carried by the previous revision of this page.*

### Variability Management

The role of adjusting the interface, guided by versioning, rests solely within architecture requirements. Thereafter, vendors are obliged to align their implementation with a designated version of the interface. As per `SLA` terms, they may transition to newer versions based on demand needs.

Each API interface will be versioned using [Semantic Versioning 2.0.0](https://semver.org/); the vendor code will comply with a specific version of the interface.

Two facts about this particular interface qualify that governance, and both are stated plainly rather than left to inference:

- **The header defines no compile-time variability flag.** There is no feature macro that adds, removes or alters a declaration or a type; every declaration is unconditional. A vendor cannot present a reduced surface and remain conformant.
- **The header defines no version macro.** Unlike HALs that publish major and minor version macros, this one publishes none, so a caller cannot query the interface version at compile time or at runtime, and cannot guard code against an interface revision. Version alignment is therefore an integration-time agreement rather than something the interface enforces.

*Derived from `include/mso_mgmt_hal.h` (no feature or version macro is defined) and the versioning statements carried by the previous revision of this page.*

### Platform or Product Customization

This interface exposes no product- or platform-conditional surface. No compile flag excludes any declaration, macro or type, so a conforming implementation presents the same three functions with the same signatures on every product.

The one variation the interface does record is implementation-side and invisible to a caller: the seed may be held in a configuration file or behind the `rdkbEncryptedClientSeed` `SNMP` `OID` (`include/mso_mgmt_hal.h:207`), and newer broadband devices are required to decrypt it on demand when `mso_set_pod_seed` is called (`:200`). Which applies is a vendor and device-class matter that this interface does not constrain and does not report.

*Derived from `include/mso_mgmt_hal.h:200, 207` and the unconditional declarations at `:187`, `:202`, `:217`.*

## Interface API Documentation

All HAL function prototypes and datatype definitions are available in the [`mso_mgmt_hal.h`](../../include/mso_mgmt_hal.h) file.

1. Components/Processes must include `mso_mgmt_hal.h` to make use of MsoMgmt HAL capabilities.
2. Components/Processes should add a linker dependency for `libhal_msomgmt.so`.

Per-API detail — argument ranges, pre-conditions, every return value and the reason for it — is carried by the inline documentation on each declaration in that header, and is not duplicated here.

*Derived from `include/mso_mgmt_hal.h` and the include-and-link direction carried by the previous revision of this page.*

### Theory of operation and key concepts

This interface exists so that RDK-B middleware can answer one question and maintain the input to it. The question is whether a password offered to the device matches the operator's password of the day; the input is the `PoD` seed from which that day's password is derived.

Two concepts carry the whole contract:

- **Password of the day** — a credential that is valid for the current day. `mso_validatepwd` "checks if the provided password matches the valid password set for the MSO user for the current day" (`include/mso_mgmt_hal.h:172`). The interface exposes no way to read or generate the password itself; a caller can only submit a candidate and receive a verdict.
- **`PoD` seed** — the value from which the daily password is generated. `mso_set_pod_seed` "configures the seed value used to generate the daily password for MSO users" (`:192`), and `mso_get_pod_seed` retrieves the decrypted seed (`:207`). The derivation function itself is not part of this interface.

The interface is a value-passing boundary, not an object model: three calls, caller-owned buffers, and a status per call.

*Derived from `include/mso_mgmt_hal.h:170-172, 190-192, 204-207`.*

#### Object Lifecycles

**There are no objects and no handles.** This interface declares no context type, no opaque pointer, no create or destroy call and no instance identifier. Nothing is opened before use and nothing is released afterwards, so there is no lifecycle for a caller to manage and no leak for a caller to cause by omission.

The caller owns every buffer that crosses the boundary and is responsible for its allocation, its lifetime and — for the seed buffer — its erasure (`include/mso_mgmt_hal.h:215`). The HAL owns nothing on the caller's behalf.

*Derived from `include/mso_mgmt_hal.h:187, 202, 215, 217` (no context type or lifecycle call is declared).*

#### Method Sequencing

The seed underpins derivation of the password of the day: `mso_set_pod_seed` "configures the seed value used to generate the daily password for MSO users" (`include/mso_mgmt_hal.h:192`) and `mso_validatepwd` matches a candidate against "the valid password set for the MSO user for the current day" (`:172`). Read together, those two statements establish a logical relationship — a seed has to be in place for a validation to succeed against a password derived from it.

That relationship is logical, not mandated, and the distinction matters to a caller:

- **The interface requires no call order.** None of the three declarations states a pre-condition on another.
- **The interface defines no initialization**, so there is no first call to make.
- **The interface does not specify the behavior of `mso_validatepwd` when no seed has been set** — not which status it returns, and not whether the call is valid at all.
- `mso_set_pod_seed` and `mso_get_pod_seed` are not documented as a matched pair. Nothing states that a seed written by the setter is the seed returned by the getter, and the getter names its own sources independently (`:207`).

A caller must therefore sequence calls according to its own requirements and treat any assumption beyond the above as unestablished.

*Derived from `include/mso_mgmt_hal.h:170-172, 190-192, 204-207`.*

#### State-Dependent Behavior

**This interface specifies no state-dependent behavior.** It defines no device state, no session state and no connection state, and no declaration is documented as valid only in a particular state or as changing the validity of another.

The values a caller can observe are per-call results, not states: `mso_pwd_ret_status` from `mso_validatepwd`, and `RETURN_OK` or `RETURN_ERR` from the two seed accessors. They describe the outcome of the call that has just returned and nothing about the device before or after it. See `State Diagram`.

*Derived from `include/mso_mgmt_hal.h:100, 104, 127-135, 187, 202, 217`.*

### Data Structures and Defines

The types a caller must construct or interpret are listed below, each with the location of its declaration. This header declares **no structure type and no callback typedef**, so there is no caller-populated structure to describe and no registration function to list.

**`mso_pwd_ret_status`** (`include/mso_mgmt_hal.h:127-135`) — a `typedef enum` and the return type of `mso_validatepwd`. It has **five** members:

| Member | Location | Meaning |
| --- | --- | --- |
| `Invalid_PWD` | `:129` | Password is invalid. |
| `Good_PWD` | `:130` | Password is valid. |
| `Unique_PWD` | `:131` | Password is unique, meaning not previously used. |
| `Expired_PWD` | `:132` | Password is expired. |
| `TimeError` | `:133` | A time-related error occurred during validation. |

A caller must handle all five. The enumeration is the type's domain, and it is worth being explicit that the header's own return-value list for `mso_validatepwd` (`:176-180`) documents only four of them, omitting `Unique_PWD`; the declaration can still return it.

**Status defines** — the domain of both seed accessors, which return `INT` rather than the enumeration above:

| Define | Location | Value | Notes |
| --- | --- | --- | --- |
| `RETURN_OK` | `:100` | `0` | Success, for `mso_set_pod_seed` and `mso_get_pod_seed`. |
| `RETURN_ERR` | `:104` | `-1` | Failure, for both seed accessors; the value both are documented against. |
| `ERROR` | `:120` | `-1` | Defined here and numerically identical to `RETURN_ERR`; referenced by no declaration in this header. |

**`SIZE_arrisCmDevHttpClientSeed`** (`:116`) — value `8L`. Two facts about it are recorded rather than reconciled, because reconciling either would require changing the interface and this change set is documentation only:

- No declaration references it, and **the interface does not establish its relationship to the "at least 64 bytes" buffer requirement** the seed accessors state at `:194` and `:209`. A caller must size seed buffers from that requirement, not from this constant.
- Its include guard is spelled `SIZE_arrisCmDevHttpClientSee` (`:115`), without the trailing `d`, so the guard does not name the macro it protects. A translation unit that pre-defines the macro under its correct name would therefore see it redefined here.

**Compatibility aliases and typedefs** — `CHAR`, `UCHAR`, `BOOLEAN`, `INT`, `UINT`, `ULONG`, `TRUE`, `FALSE` and `ENABLE` (`:63-97`), `SEC_PER_YEAR` with value `31536000` (`:112`), and the typedefs `uint16`, `uint32` and `boolean` (`:107-109`). These exist for source and `ABI` compatibility with legacy RDK-B translation units, and each is guarded so that a prior definition wins. Of the whole set, **only `INT` appears in a declared signature** — as the return type of both seed accessors (`:202`, `:217`); none of the others is referenced by any declaration in this header.

*Derived from `include/mso_mgmt_hal.h:63-135, 176-180, 194, 202, 209, 217`.*

### API Surface

Every function this interface declares is named below by exact identifier. This topic is the index of the interface: a reader who needs only an overview has it in `Description` and `Component Runtime Execution Requirements` above, while a reader with a specific question about a call starts here and follows the link into the declaration.

**Password validation** — one call, returning the `mso_pwd_ret_status` enumeration:

- `mso_validatepwd` — validates a caller-supplied password against the current MSO password of the day and returns an `mso_pwd_ret_status`. Declared at [`include/mso_mgmt_hal.h`](../../include/mso_mgmt_hal.h) `:187`.

**Password-of-the-Day seed** — two accessors, both returning the integer status domain:

- `mso_set_pod_seed` — configures the `PoD` seed used to generate the daily password; returns `RETURN_OK` or `RETURN_ERR`. Declared at [`include/mso_mgmt_hal.h`](../../include/mso_mgmt_hal.h) `:202`.
- `mso_get_pod_seed` — retrieves the decrypted `PoD` seed into a caller-supplied buffer that the caller must overwrite after use; returns `RETURN_OK` or `RETURN_ERR`. Declared at [`include/mso_mgmt_hal.h`](../../include/mso_mgmt_hal.h) `:217`.

That is the complete surface. There is no initialization call, no teardown call, no device-status call, no data-model accessor and no notification registration in this interface.

*Derived from `include/mso_mgmt_hal.h:187, 202, 217`.*

### Sequence Diagram

The exchange below uses the three declared identifiers and no others. Participants are the RDK-B caller, the HAL interface, and the vendor software behind it.

```mermaid
sequenceDiagram
    participant Caller as RDK-B Caller
    participant HAL as MsoMgmt HAL
    participant Vendor as Vendor Software
    Caller->>HAL: mso_set_pod_seed(pSeed)
    HAL->>Vendor: store the PoD seed
    Vendor-->>HAL: store result
    HAL-->>Caller: RETURN_OK or RETURN_ERR
    Caller->>HAL: mso_get_pod_seed(pSeed)
    HAL->>Vendor: read seed from config file or rdkbEncryptedClientSeed OID
    Vendor-->>HAL: decrypted seed
    HAL-->>Caller: RETURN_OK, seed written into the caller buffer
    Note over Caller: Caller overwrites pSeed after use
    Caller->>HAL: mso_validatepwd(pwd)
    HAL->>Vendor: compare candidate against the password of the day
    Vendor-->>HAL: validation outcome
    HAL-->>Caller: mso_pwd_ret_status
```

The order shown is illustrative of a seed-then-validate flow and is not a required sequence; see `Method Sequencing`.

*Derived from `include/mso_mgmt_hal.h:187, 202, 207, 215, 217`.*

### State Diagram

**No state diagram is drawn for this interface, and that absence is a finding rather than an omission.** This interface exposes status values, not states: it defines no device state, no session state and no lifecycle, and specifies no transition between any of the values it does define. Drawing a state machine would require inventing edges the interface does not establish.

The status values a caller can observe are:

- `mso_pwd_ret_status` (`include/mso_mgmt_hal.h:127-135`) — `Invalid_PWD`, `Good_PWD`, `Unique_PWD`, `Expired_PWD`, `TimeError`.
- The seed-accessor integer domain — `RETURN_OK` (`:100`) and `RETURN_ERR` (`:104`).

Each is the result of the individual call that returned it. None of them is reported as a persistent condition, none can be queried independently of making a call, and the interface does not state that any value constrains which value a subsequent call may return.

*Derived from `include/mso_mgmt_hal.h:100, 104, 127-135`.*
