/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**!
*
* @file mso_mgmt_hal.h
* @brief MSO Management Hardware Abstraction Layer (HAL)
*
* This header file provides a standardized interface for interacting with
* DOCSIS devices in Multiple System Operator (MSO) environments.
*
* The MSO Management HAL enables key functionalities like:
*   -  Device provisioning and configuration
*   - Data model access (reading/writing parameters)
*   - Event notifications for status changes
*   - Security management for device interactions
*
* By abstracting hardware and protocol specifics, this HAL simplifies
* management application development for diverse DOCSIS devices.
*
* @par Declared scope
* The paragraphs above describe the MSO deployment context rather than the surface
* this header declares. The declared contract is exactly three functions - one
* password validation call and two password-of-the-day seed accessors - and no
* declaration here provisions a device, reads or writes a data model, delivers an
* event notification, or performs any security operation other than validating a
* candidate password. A caller takes the three declarations as the whole of the
* contract; the repository specification states the same boundary under
* `Description` (`docs/pages/halSpec.md`).
*
* @par Basis of the statements in this header
* Every behavioural statement in this file is derived from these declarations and
* from that specification, which is this repository's own policy statement. Where
* neither establishes a behaviour it is recorded as unspecified rather than guessed,
* so that a caller can tell a contract from an assumption. Nothing here is presented
* as observed runtime behaviour.
*/

#ifndef __MSO_MGMT_HAL_H__
#define __MSO_MGMT_HAL_H__

/**********************************************************************
               CONSTANT DEFINITIONS
**********************************************************************/
/**
* @defgroup msomgmt_hal MSO Management HAL
*
* This module provides the function call prototypes and structure definitions.
*
* @defgroup  msomgmt_hal_types  MSO Management HAL Data Types
* @ingroup   msomgmt_hal
*
* @defgroup msomgmt_hal_apis   MSO Management HAL APIs
* @ingroup  msomgmt_hal
*
**/


/**
 * @addtogroup msomgmt_hal_types
 * @{
 */

/*
 * Legacy RDK-B source-compatibility aliases.
 *
 * The eleven aliases below and the three typedefs that follow them exist so that
 * translation units carrying the historical RDK-B spellings compile against this
 * header. Each of the eleven aliases is guarded, so a definition already in effect
 * wins and this header does not redefine it. The three typedefs carry no guard, so a
 * translation unit that also defines uint16, uint32 or boolean as a type depends on
 * the compiler accepting a repeated typedef - which C11 permits only where the two
 * definitions are identical, and earlier standards do not permit at all.
 *
 * Of the whole set only INT is referenced by a declaration in this file, as the return
 * type of both seed accessors; the rest are provided for compatibility and appear in
 * no signature this interface declares.
 */

#ifndef CHAR
#define CHAR  char /*!< Alias for the C `char` type, provided for legacy RDK-B translation units. Not used by any declaration in this header. */
#endif

#ifndef UCHAR
#define UCHAR unsigned char /*!< Alias for `unsigned char`, provided for legacy RDK-B translation units. Not used by any declaration in this header. */
#endif

#ifndef BOOLEAN
#define BOOLEAN  unsigned char /*!< Alias for the byte-wide boolean type used by legacy RDK-B code; holds TRUE or FALSE. Not used by any declaration in this header. */
#endif

#ifndef INT
#define INT   int /*!< Alias for the C `int` type. This is the declared return type of mso_set_pod_seed() and mso_get_pod_seed(), whose value domain is RETURN_OK and RETURN_ERR. */
#endif

#ifndef UINT
#define UINT  unsigned int /*!< Alias for `unsigned int`, provided for legacy RDK-B translation units. Not used by any declaration in this header. */
#endif

#ifndef ULONG
#define ULONG unsigned long /*!< Alias for `unsigned long`, provided for legacy RDK-B translation units. Not used by any declaration in this header. */
#endif

#ifndef TRUE
#define TRUE     1 /*!< Boolean true for the BOOLEAN and boolean aliases above. Not used by any declaration in this header. */
#endif

#ifndef FALSE
#define FALSE    0 /*!< Boolean false for the BOOLEAN and boolean aliases above. Not used by any declaration in this header. */
#endif

#ifndef ENABLE
#define ENABLE   1 /*!< Legacy "enabled" constant, numerically identical to TRUE. No declaration in this header takes or returns it. */
#endif

#ifndef RETURN_OK
#define RETURN_OK   0 /*!< Success status of mso_set_pod_seed() and mso_get_pod_seed(). A caller compares the returned INT against this value; it is not the success value of mso_validatepwd(), which returns mso_pwd_ret_status instead. */
#endif

#ifndef RETURN_ERR
#define RETURN_ERR   -1 /*!< Failure status of mso_set_pod_seed() and mso_get_pod_seed(). It is the only failure value either function defines, so it aggregates every failure cause and a caller cannot derive the cause from it. */
#endif

/*! Unsigned 16-bit integer alias retained for legacy RDK-B translation units. No declaration in this header takes or returns it. */
typedef unsigned short  uint16;
/*! Unsigned integer alias retained for legacy RDK-B translation units. Its width follows `unsigned long` on the build platform and is therefore not guaranteed to be 32 bits. No declaration in this header takes or returns it. */
typedef unsigned long   uint32;
/*! Signed integer alias used by legacy RDK-B code to carry TRUE or FALSE. No declaration in this header takes or returns it. */
typedef int             boolean;

#ifndef SEC_PER_YEAR
#define SEC_PER_YEAR    31536000 /*!< Seconds in a 365-day year (365 * 24 * 60 * 60). Provided for legacy callers computing credential ages; no declaration in this header takes or returns it, and this interface states no relationship between it and the daily validity period of a password. */
#endif

/*
 * Interface defect: the guard below and the macro it is meant to protect do not carry
 * the same name.
 *
 *   * The guard tests SIZE_arrisCmDevHttpClientSee - no trailing "d".
 *   * The macro the body defines is SIZE_arrisCmDevHttpClientSeed, with the "d".
 *
 * Two consequences follow, and a caller has to work around both rather than rely on
 * the guard doing its job:
 *
 *   1. The guard can never suppress this definition. Defining
 *      SIZE_arrisCmDevHttpClientSeed before including this header does not prevent
 *      the definition below, so a translation unit that pre-defines it under the
 *      correct name sees it redefined here - a warning, or an error under a strict
 *      compiler, if the two definitions do not match token for token.
 *   2. The only name that is actually defined is SIZE_arrisCmDevHttpClientSeed.
 *      That is the name a caller tests with #ifdef and uses in code;
 *      SIZE_arrisCmDevHttpClientSee is never defined by this header, so testing it
 *      is always false and must not be used as a feature test.
 *
 * The repository specification records the same defect under `Data Structures and
 * Defines` (`docs/pages/halSpec.md`).
 */
#ifndef SIZE_arrisCmDevHttpClientSee
#define SIZE_arrisCmDevHttpClientSeed 8L /*!< Legacy element count, value 8L, carried for source compatibility with the Arris HTTP client seed field. No declaration in this header references it, and this interface does not relate it to the "at least 64 bytes" buffer requirement the seed accessors state, so a caller sizes seed buffers from that requirement and not from this constant. */
#endif

#ifndef ERROR
#define ERROR -1 /*!< Legacy failure constant, numerically identical to RETURN_ERR. No declaration in this header returns it, and it is not part of either return domain this interface defines; it is retained for translation units that already use the spelling. */
#endif

/**********************************************************************
                STRUCTURE DEFINITIONS
**********************************************************************/
/**
* @brief Verdict returned by mso_validatepwd() for a candidate MSO password.
*
* These five enumerators are the complete domain of the function's return value, and
* a caller must handle all five: this interface defines no other value and no
* out-of-band error channel for that call. Only Good_PWD authorises a caller to
* proceed; every other value denies the candidate, and the value alone does not tell
* a caller why. The enumerators are unprefixed and therefore occupy the global
* enumerator namespace of every translation unit that includes this header.
*
* @see mso_validatepwd
*/
typedef enum
{
    Invalid_PWD, /*!< The candidate does not match the password of the day. A caller denies the request. This value carries no cause: a wrong candidate, an absent or unusable seed and a vendor-side validation failure are all reported the same way. */
    Good_PWD,    /*!< The candidate matches the password of the day. This is the only value that authorises a caller to proceed. */
    Unique_PWD,  /*!< The candidate is unique, meaning not previously used. This interface does not state the condition under which mso_validatepwd() returns it, so a caller must not read it as success; it is a distinct outcome that is not Good_PWD and a caller denies the request. */
    Expired_PWD, /*!< The candidate is recognised but is no longer valid for the current day. A caller denies the request and obtains a current password rather than retrying the same value. */
    TimeError    /*!< Validation could not be completed because of a time-related failure, for example a device clock that is not synchronised. The verdict is inconclusive rather than a rejection: a caller denies the request for now and may retry once time is available. */
}
mso_pwd_ret_status;

/*
 * Two limits of the enumeration above, stated so that a caller does not infer
 * capabilities it does not have.
 *
 *   * There is no "unknown" or "not validated" value. Every call returns one of the
 *     five verdicts, so a caller cannot distinguish "validation did not run" from
 *     "validation ran and rejected the candidate" through this type. A caller that
 *     needs that distinction has to track it itself around the call.
 *   * There is no value that reports the validation service as unavailable. A seed
 *     that is absent, undecryptable or unreadable surfaces through the same
 *     enumerators as a wrong password, so this type cannot be used to tell an
 *     authentication failure from a service failure.
 *
 * The enumerator names are also part of the interface's ABI-visible surface: their
 * ordinals are the values an implementation returns, so neither their spelling nor
 * their order can change without breaking existing callers.
 */

/**
 * @}
 */

/**
 * @addtogroup msomgmt_hal_apis
 * @{
 */

/*
 * Return-code contract for the three functions below.
 *
 * Every failure is reported synchronously through the return value. This interface
 * declares no error-detail accessor, sets no errno convention, and delivers no
 * asynchronous notification, so the returned value is the whole of the diagnosis a
 * caller receives. The repository specification states the same under `Internal
 * Error Handling` (`docs/pages/halSpec.md`).
 *
 * There are exactly two return domains:
 *
 *   * mso_pwd_ret_status, for mso_validatepwd(). Five verdicts, documented on the
 *     enumeration above and on the declaration below.
 *   * INT carrying RETURN_OK or RETURN_ERR, for both seed accessors. RETURN_ERR is
 *     the single failure value each of them defines, so it aggregates every failure
 *     cause: a caller cannot tell a rejected or malformed value from a decryption
 *     failure, an unreadable source or an internal fault, and no further call
 *     narrows it down.
 *
 * Neither domain defines a value specific to an invalid argument, and neither states
 * how an implementation responds to one: nothing here establishes that such a call is
 * rejected, and nothing here establishes that it cannot be. A caller therefore
 * validates its own arguments before the call rather than relying on a rejection it
 * cannot count on, and must not read a failure value as a report about the arguments.
 *
 * There is also no initialisation or teardown call in this interface and no context
 * handle, so no failure code below means "not initialised" and there is no state a
 * caller can re-establish after one. Each of the three functions is called directly,
 * and a failure is retried, reported or abandoned on the caller's own terms.
 */

/**
* @brief Validates a candidate password against the current MSO password of the day.
*
* Compares the value the caller supplies with the password that is valid for the MSO
* user for the current day, and returns a verdict. The password of the day is derived
* from the seed installed through mso_set_pod_seed(); the derivation itself is not
* part of this interface, and no call here reads or generates the password, so a
* caller can only submit a candidate and act on the verdict.
*
* @param[in] pwd - Caller-allocated candidate password to validate; the caller
*                  allocates the buffer, owns it and releases it. This interface
*                  states no maximum length, no permitted character set and no
*                  capacity, and the declaration carries no length argument, so a
*                  caller must supply a NUL-terminated string: the terminator is a
*                  caller obligation and not a behaviour this interface states.
*                  What an implementation does with a buffer that carries no
*                  terminator is not stated either - no verdict is defined for an
*                  invalid argument, and this interface neither establishes that
*                  such a call is rejected nor establishes that it cannot be - so
*                  a caller must not rely on a rejection. The parameter is
*                  declared `char *` rather than `const char *`, and whether the
*                  implementation writes through it or retains the pointer after
*                  returning is not stated, so a caller must not share the buffer
*                  with another thread for the duration of the call and must not
*                  assume its contents are intact afterwards. It must not be NULL;
*                  the precondition below states that obligation and what this
*                  interface does and does not establish about a violation of it,
*                  and the warnings below state the handling the value requires
*                  because it is a credential.
*
* @pre None imposed by this interface beyond the argument itself: no
*      initialisation call is declared, so there is nothing to call first, and no
*      call order is required (`Initialization and Startup` and `Method
*      Sequencing` in `docs/pages/halSpec.md`). `pwd` must address a
*      NUL-terminated string. If it does not - a NULL pointer, freed storage or an
*      unterminated buffer - this interface states no outcome: no verdict is
*      defined for an invalid argument, and it neither states that such a call is
*      rejected nor states that it cannot be, so the verdict returned and any
*      effect on the caller's buffer are both unspecified. A caller validates the
*      argument itself rather than relying on a rejection, and does not read a
*      verdict as a report about the argument. A seed must logically be in place
*      for a candidate derived from it to validate, but this interface does not
*      require mso_set_pod_seed() to have been called and does not specify which
*      verdict is returned when no seed has been installed.
*
* @post The call has no output parameter and changes no state a caller can observe:
*       the verdict is the entire result. On every return, including a failure
*       verdict, whether the implementation modified the caller's buffer is
*       unspecified, so a caller re-establishes the buffer's contents itself if it
*       needs them again. Whether the caller may safely erase that buffer once the
*       call has returned is a separate question this interface does not settle; the
*       warnings below state what it does and does not establish.
*
* @returns mso_pwd_ret_status - the verdict, reported synchronously. The enumeration
*          is the complete domain and a caller must handle all five values.
* @retval Good_PWD - The candidate matches the password of the day. This is the only
*         value that authorises the caller to proceed.
* @retval Invalid_PWD - The candidate does not match. The caller denies the request.
*         No cause is conveyed; the note below on indistinguishable failures states
*         what this value does and does not establish.
* @retval Expired_PWD - The candidate is recognised but is no longer valid for the
*         current day. The caller denies the request and obtains a current password;
*         retrying the same value cannot succeed.
* @retval TimeError - Validation could not be completed because of a time-related
*         failure, for example an unsynchronised device clock. The caller treats the
*         result as inconclusive rather than as a rejection, denies the request for
*         now, and may retry once time is available.
* @retval Unique_PWD - Defined by mso_pwd_ret_status as "password is unique, meaning
*         not previously used". This interface does not state the condition under
*         which an implementation returns it from this function, so a caller must not
*         read it as success: it handles it as an outcome that is not Good_PWD and
*         denies the request.
*
* @note A failure verdict does not distinguish a wrong password from an unavailable
*       validation backend. There is one verdict enumeration, no error-detail
*       accessor and no errno convention, so an absent, undecryptable or unreadable
*       seed and a vendor-side fault surface through the same values as a genuinely
*       wrong candidate - Invalid_PWD, or TimeError where a clock is implicated. A
*       caller that must tell an authentication failure from a service failure cannot
*       derive it from this return value and has to obtain it elsewhere, for example
*       by checking with mso_get_pod_seed() that a seed is retrievable at all.
* @note Blocking behaviour is not specified. This interface does not state whether
*       this call may block, and a caller must not assume either behaviour. No
*       response-time budget or timeout value is specified either, so a
*       caller that cannot tolerate an unbounded wait treats the call as potentially
*       blocking and imposes its own bound.
* @note Thread safety is not provided. There is no requirement on an implementation
*       to be thread safe, and the calling module is obliged to serialise its calls
*       into this HAL (`Threading Model` in `docs/pages/halSpec.md`). The functions
*       are expected to be reachable from several processes, so a vendor
*       implementation must also protect shared state across process boundaries
*       (`Process Model`); a caller cannot infer cross-process serialisation from its
*       own locking.
*
* @warning The candidate is a credential. It must not be logged, echoed into an error
*          or trace message, or copied into longer-lived, shared or globally reachable
*          storage.
* @warning Erasing the caller's buffer is a separate matter, and this interface does
*          not establish a point at which it is safe. It does not state whether the
*          implementation retains `pwd` after returning, and it declares no wipe,
*          release or completion call through which a retained pointer could be
*          withdrawn, so a return does not establish that the storage has become
*          private to the caller again. A caller whose own policy requires the
*          candidate to be erased therefore has to obtain the guarantee that makes the
*          erase safe - that the implementation consumes the value synchronously and
*          retains nothing - from the implementation it runs against; this interface
*          does not give it, and until it is in hand an overwrite performed as soon as
*          the call returns may write storage the implementation is still reading.
*          Where that guarantee is in hand, the overwrite has to be a write the
*          compiler cannot elide - through a volatile pointer, or the platform's
*          explicit memory-clear routine - because a plain clear immediately before
*          release may be optimised away.
*
* @see mso_pwd_ret_status
* @see mso_set_pod_seed
* @see mso_get_pod_seed
*/
mso_pwd_ret_status mso_validatepwd(char *pwd);

/**
* @brief Installs the Password of the Day (PoD) seed used to derive MSO passwords.
*
* Configures the seed value from which the daily password for MSO users is generated.
* The derivation function is not part of this interface and no call reports the
* resulting password, so the effect of a successful call is observable only through
* the verdicts mso_validatepwd() subsequently returns. Where the seed is held, and
* whether it survives a restart, is a vendor matter this interface does not specify
* (`Persistence Model` in `docs/pages/halSpec.md`). On newer broadband devices the
* implementation is required to decrypt the seed on demand when this function is
* called; whether a given device is in that class is not reported by this interface,
* so a caller cannot tell from the return value whether decryption took place.
*
* @param[in] pSeed - Caller-allocated buffer holding the seed to install, and at
*                    least 64 bytes long. The caller allocates the buffer, owns it and
*                    releases it. Sixty-four bytes is the only capacity this interface
*                    states; the declaration carries no length argument, so the
*                    implementation cannot be told that the buffer is smaller. What
*                    happens if it is shorter is not stated: no status value is defined
*                    for an invalid argument, and this interface neither establishes
*                    that such a call is rejected nor establishes that it cannot be, so
*                    a caller must not rely on a rejection and must not read RETURN_ERR
*                    as a report about the buffer. Whatever an implementation reads
*                    beyond the end of a shorter buffer is storage the caller did not
*                    provide, which is why the 64 bytes are a caller obligation rather
*                    than advice, and why a caller cannot delegate the check to this
*                    call. That capacity is also the whole of what this interface says
*                    about the argument: this interface does not state whether the
*                    implementation reads a NUL-terminated string or a fixed 64-byte
*                    field, and states no encoding, so the representation of the seed
*                    and the length of the meaningful data inside the buffer are both
*                    unspecified. Neither follows from the 64-byte figure, which bounds
*                    the storage the caller must provide and says nothing about the
*                    value inside it: no maximum value length is stated, no terminator
*                    is required, and it is not established that a value shorter than
*                    the buffer is recognised or that trailing bytes are ignored. A
*                    caller therefore cannot derive a seed representation from this
*                    interface and agrees it with the implementation it runs against.
*                    This interface also does not state whether the implementation
*                    retains the pointer beyond the call, and declares no call through
*                    which a retained pointer could be withdrawn, so a caller must not
*                    rely on the buffer becoming private again on return and must not
*                    assume that erasing its own copy erases the seed the
*                    implementation now holds. It must not be NULL. The warnings below
*                    state the handling the value requires because it is a credential.
*
* @pre None imposed by this interface beyond the argument itself: no initialisation
*      call is declared, so there is nothing to call first, and no call order is
*      required. `pSeed` must address at least 64 readable bytes holding the seed. If
*      it does not - a NULL pointer, freed storage or a shorter buffer - this interface
*      states no outcome: no status value is defined for an invalid argument, and it
*      neither states that such a call is rejected nor states that it cannot be, so both
*      the value returned and the seed that is in effect afterwards are unspecified. A
*      caller validates the argument itself rather than relying on a rejection.
*
* @post On RETURN_OK the seed has been accepted as the value from which the daily
*       password is derived. This interface does not state that the seed accepted here
*       is the seed mso_get_pod_seed() returns, and does not state what becomes of a
*       previously installed seed, so neither may be assumed. On RETURN_ERR the seed
*       that is in effect is unspecified: this interface does not state that the
*       operation is atomic, so a caller must not conclude that the previous seed
*       still stands, and re-installs a known value before relying on validation
*       again.
*
* @returns INT - the outcome, reported synchronously. RETURN_OK and RETURN_ERR are the
*          only values this interface defines for it.
* @retval RETURN_OK - The seed was accepted.
* @retval RETURN_ERR - The seed was not accepted. This one value covers every failure
*         cause - an invalid seed and a decryption error are both documented against
*         it, and an internal or storage fault is reported the same way - so a caller
*         cannot tell a rejected value from a vendor-side failure and no further call
*         narrows it down. The caller re-checks the value it supplied, logs the
*         failure without logging the seed, and may retry; a failure that persists is
*         an integration matter for the owner of the vendor implementation rather than
*         something a caller can work around.
*
* @note Blocking behaviour is not specified. This interface does not state whether
*       this call may block, and a caller must not assume either behaviour; the
*       repository specification records it as unspecified under `Blocking calls`. No
*       response-time budget or timeout value is specified, so a caller that cannot
*       tolerate an unbounded wait imposes its own bound. On-demand decryption is a
*       documented obligation on some devices and is a plausible source of latency,
*       which is a further reason not to assume this call returns promptly.
* @note Thread safety is not provided. There is no requirement on an implementation to
*       be thread safe, and the calling module is obliged to serialise its calls into
*       this HAL (`Threading Model` in `docs/pages/halSpec.md`). The functions are
*       expected to be reachable from several processes, so a vendor implementation
*       must also protect shared state across process boundaries (`Process Model`).
*       Two concurrent installs, or an install concurrent with a validation, are not
*       ordered by this interface.
*
* @warning The seed is a credential from which every daily password is derived, so it
*          is more sensitive than any single password. It must not be logged, written
*          to a trace, or copied into longer-lived or shared storage.
* @warning Erasing the caller's buffer is a separate matter, and this interface does
*          not establish a point at which it is safe. It does not state whether the
*          implementation retains `pSeed` beyond the call, and it declares no wipe,
*          release or completion call through which a retained pointer could be
*          withdrawn, so a return - on the success path or the failure path - does not
*          establish that the storage has become private to the caller again. A caller
*          whose own policy requires the seed to be erased therefore has to obtain the
*          guarantee that makes the erase safe - that the implementation copies the
*          value synchronously and retains nothing - from the implementation it runs
*          against; this interface does not give it, and until it is in hand an
*          overwrite performed as soon as the call returns may write storage the
*          implementation is still reading. Where that guarantee is in hand, the
*          overwrite has to be a write the compiler cannot elide, through a volatile
*          pointer or the platform's explicit memory-clear routine, because a plain
*          clear immediately before release may be optimised away. Erasing the caller's
*          copy does not erase any copy the implementation holds, and this interface
*          declares no call that does.
*
* @see mso_get_pod_seed
* @see mso_validatepwd
*/
INT mso_set_pod_seed(char *pSeed);

/**
* @brief Retrieves the decrypted Password of the Day (PoD) seed into a caller buffer.
*
* Reads the PoD seed and writes it, decrypted, into the buffer the caller supplies.
* The implementation obtains it either from the device configuration file or from the
* `rdkbEncryptedClientSeed` SNMP OID; this interface does not select between the two
* sources and the return value does not report which was used (`Optional Components`
* in `docs/pages/halSpec.md`). What the caller holds on success is therefore a
* cleartext credential, and the erasure obligation stated in the warnings below is
* part of the contract rather than advice - together with the guarantee a caller needs
* from its implementation before that erasure is safe, which this interface does not
* itself give.
*
* @param[out] pSeed - Caller-allocated buffer of at least 64 bytes that receives the
*                     decrypted seed. The caller allocates the buffer, owns it and
*                     releases it; the implementation writes into it, and whether it
*                     retains the pointer beyond the call is unspecified - this
*                     interface neither states that it does nor states that it does
*                     not, and declares no call through which a retained pointer could
*                     be withdrawn. Sixty-four bytes is the only capacity this
*                     interface states; the declaration carries no length
*                     argument, so the implementation cannot be told that
*                     the buffer is smaller. What happens if it is shorter is not
*                     stated: no status value is defined for an invalid argument, and
*                     this interface neither establishes that such a call is rejected
*                     nor establishes that it cannot be, so a caller must not rely on
*                     a rejection and must not read RETURN_ERR as a report about the
*                     buffer. Whatever the implementation writes past the end of a
*                     shorter buffer lands outside the caller's own allocation, which
*                     is why at least 64 writable bytes are a caller obligation rather
*                     than advice, and why a caller cannot delegate the check to this
*                     call. This interface does not state whether the implementation
*                     writes a NUL terminator, how many bytes it writes, or how a
*                     caller determines the length of the value it has received, so a
*                     caller must not treat the result as a C string by assumption:
*                     zero the whole buffer before the call, and bound every
*                     subsequent read by the 64 bytes the buffer holds rather than by
*                     a terminator it may not contain. It must not be NULL.
*
* @pre None imposed by this interface beyond the argument itself: no initialisation
*      call is declared, so there is nothing to call first, and no call order is
*      required - in particular mso_set_pod_seed() need not have been called, and this
*      interface does not state that the seed returned here is the one installed
*      there. `pSeed` must address at least 64 writable bytes. If it does not - a NULL
*      pointer, freed storage, read-only storage or a shorter buffer - this interface
*      states no outcome: no status value is defined for an invalid argument, and it
*      neither states that such a call is rejected nor states that it cannot be, so both
*      the value returned and the state of the caller's buffer afterwards are
*      unspecified. A caller validates the argument itself rather than relying on a
*      rejection.
*
* @post On RETURN_OK the buffer holds the decrypted seed, subject to the termination
*       caveat above, and the caller is holding a cleartext credential that this
*       interface requires it to erase once it has used it. On RETURN_ERR the buffer
*       contents are indeterminate and must not be read as a seed: this interface does
*       not state whether the implementation wrote anything before failing, so a caller
*       assumes it may have and the same erasure obligation applies on the failure path
*       as on the success path. Both warnings below apply to that erasure: the second
*       states the guarantee a caller needs before performing it, which this interface
*       does not itself give.
*
* @returns INT - the outcome, reported synchronously. RETURN_OK and RETURN_ERR are the
*          only values this interface defines for it.
* @retval RETURN_OK - A seed was retrieved and written into the caller's buffer.
* @retval RETURN_ERR - No usable seed was written. This one value covers every failure
*         cause - a retrieval error and a decryption error are both documented against
*         it, and an absent seed, an unreadable configuration file and an unavailable
*         SNMP source are reported the same way - so a caller cannot tell which
*         occurred and no further call narrows it down. The caller treats the seed as
*         unavailable, erases the buffer under the warnings below, and logs the failure
*         without logging any buffer content.
*
* @note Blocking behaviour is not specified. This interface does not state whether
*       this call may block, and a caller must not assume either behaviour; the
*       repository specification records it as unspecified under `Blocking calls`. No
*       response-time budget or timeout value is specified, so a caller that cannot
*       tolerate an unbounded wait imposes its own bound. Both documented sources - a
*       configuration file and an SNMP OID - and the decryption step are plausible
*       sources of latency.
* @note Thread safety is not provided. There is no requirement on an implementation to
*       be thread safe, and the calling module is obliged to serialise its calls into
*       this HAL (`Threading Model` in `docs/pages/halSpec.md`). The functions are
*       expected to be reachable from several processes, so a vendor implementation
*       must also protect shared state across process boundaries (`Process Model`). A
*       retrieval concurrent with mso_set_pod_seed() is not ordered by this interface,
*       so which seed is returned in that case is unspecified.
*
* @warning On success the caller's buffer holds a decrypted secret from which every
*          daily password can be derived. It must be overwritten once it has been
*          used, with a write the compiler cannot elide - through a volatile pointer or
*          the platform's explicit memory-clear routine, because a plain clear
*          immediately before release may be optimised away. Until then it must not be
*          logged, written to a trace or an error message, copied into a longer-lived,
*          shared or globally reachable allocation, or left in storage that a crash
*          dump would capture. Erasing the caller's copy does not erase whatever copy
*          the implementation holds; this interface provides no call that does.
* @warning This interface states the obligation above but does not establish a point at
*          which discharging it is safe with respect to the implementation. Whether the
*          implementation retains `pSeed` beyond the call is unspecified, and no wipe,
*          release or completion call is declared through which a retained pointer
*          could be withdrawn, so a return does not establish that the storage has
*          become private to the caller again. A caller has to obtain the guarantee
*          that makes the overwrite safe - that the implementation writes the value
*          synchronously and retains no pointer to the buffer - from the implementation
*          it runs against; this interface does not give it, and until it is in hand an
*          overwrite performed as soon as the call returns may write storage the
*          implementation is still accessing. That guarantee is an integration matter
*          for the owner of the vendor implementation, and a caller that cannot obtain
*          it holds a cleartext credential it can neither safely erase nor rely on this
*          interface to erase for it.
*
* @see mso_set_pod_seed
* @see mso_validatepwd
*/
INT mso_get_pod_seed(char* pSeed);

/**
 * @}
 */

#endif
