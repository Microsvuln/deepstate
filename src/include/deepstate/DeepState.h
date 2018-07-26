/*
 * Copyright (c) 2017 Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_INCLUDE_DEEPSTATE_DEEPSTATE_H_
#define SRC_INCLUDE_DEEPSTATE_DEEPSTATE_H_

#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <deepstate/Log.h>
#include <deepstate/Compiler.h>
#include <deepstate/Option.h>
#include <deepstate/Stream.h>

#ifdef assert
# undef assert
#endif

#define assert DeepState_Assert
#define assume DeepState_Assume
#define check DeepState_Check

#define MAYBE(...) \
    if (DeepState_Bool()) { \
      __VA_ARGS__ ; \
    }

DEEPSTATE_BEGIN_EXTERN_C

DECLARE_string(input_test_dir);
DECLARE_string(input_test_file);
DECLARE_string(input_test_files_dir);
DECLARE_string(input_which_test);
DECLARE_string(output_test_dir);

DECLARE_bool(take_over);
DECLARE_bool(abort_on_fail);

enum {
  DeepState_InputSize = 8192
};

/* Byte buffer that will contain symbolic data that is used to supply requests
 * for symbolic values (e.g. `int`s). */
extern volatile uint8_t DeepState_Input[DeepState_InputSize];

/* Index into the `DeepState_Input` array that tracks how many input bytes have
 * been consumed. */
extern uint32_t DeepState_InputIndex;

/* Return a symbolic value of a given type. */
extern int DeepState_Bool(void);
extern size_t DeepState_Size(void);
extern uint64_t DeepState_UInt64(void);
extern int64_t DeepState_Int64(void);
extern uint32_t DeepState_UInt(void);
extern int32_t DeepState_Int(void);
extern uint16_t DeepState_UShort(void);
extern int16_t DeepState_Short(void);
extern uint8_t DeepState_UChar(void);
extern int8_t DeepState_Char(void);

/* Returns the minimum satisfiable value for a given symbolic value, given
 * the constraints present on that value. */
extern uint32_t DeepState_MinUInt(uint32_t);
extern int32_t DeepState_MinInt(int32_t);

extern uint32_t DeepState_MaxUInt(uint32_t);
extern int32_t DeepState_MaxInt(int32_t);

DEEPSTATE_INLINE static uint16_t DeepState_MinUShort(uint16_t v) {
  return DeepState_MinUInt(v);
}

DEEPSTATE_INLINE static uint8_t DeepState_MinUChar(uint8_t v) {
  return (uint8_t) DeepState_MinUInt(v);
}

DEEPSTATE_INLINE static int16_t DeepState_MinShort(int16_t v) {
  return (int16_t) DeepState_MinInt(v);
}

DEEPSTATE_INLINE static int8_t DeepState_MinChar(int8_t v) {
  return (int8_t) DeepState_MinInt(v);
}

DEEPSTATE_INLINE static uint16_t DeepState_MaxUShort(uint16_t v) {
  return (uint16_t) DeepState_MaxUInt(v);
}

DEEPSTATE_INLINE static uint8_t DeepState_MaxUChar(uint8_t v) {
  return (uint8_t) DeepState_MaxUInt(v);
}

DEEPSTATE_INLINE static int16_t DeepState_MaxShort(int16_t v) {
  return (int16_t) DeepState_MaxInt(v);
}

DEEPSTATE_INLINE static int8_t DeepState_MaxChar(int8_t v) {
  return (int8_t) DeepState_MaxInt(v);
}

/* Returns `1` if `expr` is true, and `0` otherwise. This is kind of an indirect
 * way to take a symbolic value, introduce a fork, and on each size, replace its
* value with a concrete value. */
extern int DeepState_IsTrue(int expr);

/* Always returns `1`. */
extern int DeepState_One(void);

/* Always returns `0`. */
extern int DeepState_Zero(void);

/* Always returns `0`. */
extern int DeepState_ZeroSink(int);

/* Symbolize the data in the exclusive range `[begin, end)`. */
extern void DeepState_SymbolizeData(void *begin, void *end);

/* Concretize some data in exclusive the range `[begin, end)`. Returns a
 * concrete pointer to the beginning of the concretized data. */
extern void *DeepState_ConcretizeData(void *begin, void *end);

/* Return a symbolic C string of length `len`. */
extern char *DeepState_CStr(size_t len);

/* Symbolize a C string */
void DeepState_SymbolizeCStr(char *begin);

/* Concretize a C string. Returns a pointer to the beginning of the
 * concretized C string. */
extern const char *DeepState_ConcretizeCStr(const char *begin);

/* Allocate and return a pointer to `num_bytes` symbolic bytes. */
extern void *DeepState_Malloc(size_t num_bytes);

#define DEEPSTATE_MAKE_SYMBOLIC_ARRAY(Tname, tname) \
    DEEPSTATE_INLINE static \
    tname *DeepState_Symbolic ## Tname ## Array(size_t num_elms) { \
      tname *arr = (tname *) malloc(sizeof(tname) * num_elms); \
      DeepState_SymbolizeData(arr, &(arr[num_elms])); \
      return arr; \
    }

DEEPSTATE_MAKE_SYMBOLIC_ARRAY(Int64, int64_t)
DEEPSTATE_MAKE_SYMBOLIC_ARRAY(UInt64, uint64_t)
DEEPSTATE_MAKE_SYMBOLIC_ARRAY(Int, int)
DEEPSTATE_MAKE_SYMBOLIC_ARRAY(UInt, uint32_t)
DEEPSTATE_MAKE_SYMBOLIC_ARRAY(Short, int16_t)
DEEPSTATE_MAKE_SYMBOLIC_ARRAY(UShort, uint16_t)
DEEPSTATE_MAKE_SYMBOLIC_ARRAY(Char, char)
DEEPSTATE_MAKE_SYMBOLIC_ARRAY(UChar, unsigned char)

#undef DEEPSTATE_MAKE_SYMBOLIC_ARRAY

/* Creates an assumption about a symbolic value. Returns `1` if the assumption
 * can hold and was asserted. */
extern void _DeepState_Assume(int expr, const char *expr_str, const char *file,
                              unsigned line);

#define DeepState_Assume(x) _DeepState_Assume(!!(x), #x, __FILE__, __LINE__)

/* Result of a single forked test run.
 *
 * Will be passed to the parent process as an exit code. */
enum DeepState_TestRunResult {
  DeepState_TestRunPass = 0,
  DeepState_TestRunFail = 1,
  DeepState_TestRunCrash = 2,
  DeepState_TestRunAbandon = 3,
};

/* Abandon this test. We've hit some kind of internal problem. */
DEEPSTATE_NORETURN
extern void DeepState_Abandon(const char *reason);

/* Mark this test as having crashed. */
extern void DeepState_Crash(void);

DEEPSTATE_NORETURN
extern void DeepState_Fail(void);

/* Mark this test as failing, but don't hard exit. */
extern void DeepState_SoftFail(void);

DEEPSTATE_NORETURN
extern void DeepState_Pass(void);

/* Asserts that `expr` must hold. If it does not, then the test fails and
 * immediately stops. */
DEEPSTATE_INLINE static void DeepState_Assert(int expr) {
  if (!expr) {
    DeepState_Fail();
  }
}

/* Asserts that `expr` must hold. If it does not, then the test fails, but
 * nonetheless continues on. */
DEEPSTATE_INLINE static void DeepState_Check(int expr) {
  if (!expr) {
    DeepState_SoftFail();
  }
}

/* Return a symbolic value in a the range `[low_inc, high_inc]`. */
#define DEEPSTATE_MAKE_SYMBOLIC_RANGE(Tname, tname) \
    DEEPSTATE_INLINE static tname DeepState_ ## Tname ## InRange( \
        tname low, tname high) { \
      tname x = DeepState_ ## Tname(); \
      (void) DeepState_Assume(low <= x && x <= high); \
      return x; \
    }

DEEPSTATE_MAKE_SYMBOLIC_RANGE(Size, size_t)
DEEPSTATE_MAKE_SYMBOLIC_RANGE(Int64, int64_t)
DEEPSTATE_MAKE_SYMBOLIC_RANGE(UInt64, uint64_t)
DEEPSTATE_MAKE_SYMBOLIC_RANGE(Int, int)
DEEPSTATE_MAKE_SYMBOLIC_RANGE(UInt, uint32_t)
DEEPSTATE_MAKE_SYMBOLIC_RANGE(Short, int16_t)
DEEPSTATE_MAKE_SYMBOLIC_RANGE(UShort, uint16_t)
DEEPSTATE_MAKE_SYMBOLIC_RANGE(Char, char)
DEEPSTATE_MAKE_SYMBOLIC_RANGE(UChar, unsigned char)

#undef DEEPSTATE_MAKE_SYMBOLIC_RANGE

/* Predicates to check whether or not a particular value is symbolic */
extern int DeepState_IsSymbolicUInt(uint32_t x);

/* The following predicates are implemented in terms of `DeepState_IsSymbolicUInt`.
 * This simplifies the portability of hooking this predicate interface across
 * architectures, because basically all hooking mechanisms know how to get at
 * the first integer argument. Passing in floating point values, or 64-bit
 * integers on 32-bit architectures, can be more subtle. */

DEEPSTATE_INLINE static int DeepState_IsSymbolicInt(int x) {
  return DeepState_IsSymbolicUInt((uint32_t) x);
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicUShort(uint16_t x) {
  return DeepState_IsSymbolicUInt((uint32_t) x);
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicShort(int16_t x) {
  return DeepState_IsSymbolicUInt((uint32_t) (uint16_t) x);
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicUChar(unsigned char x) {
  return DeepState_IsSymbolicUInt((uint32_t) x);
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicChar(char x) {
  return DeepState_IsSymbolicUInt((uint32_t) (unsigned char) x);
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicUInt64(uint64_t x) {
  return DeepState_IsSymbolicUInt((uint32_t) x) ||
         DeepState_IsSymbolicUInt((uint32_t) (x >> 32U));
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicInt64(int64_t x) {
  return DeepState_IsSymbolicUInt64((uint64_t) x);
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicBool(int x) {
  return DeepState_IsSymbolicInt(x);
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicFloat(float x) {
  return DeepState_IsSymbolicUInt(*((uint32_t *) &x));
}

DEEPSTATE_INLINE static int DeepState_IsSymbolicDouble(double x) {
  return DeepState_IsSymbolicUInt64(*((uint64_t *) &x));
}

/* Used to define the entrypoint of a test case. */
#define DeepState_EntryPoint(test_name) \
    _DeepState_EntryPoint(test_name, __FILE__, __LINE__)

/* Contains information about a test case */
struct DeepState_TestInfo {
  struct DeepState_TestInfo *prev;
  void (*test_func)(void);
  const char *test_name;
  const char *file_name;
  unsigned line_number;
};

struct DeepState_TestRunInfo {
  struct DeepState_TestInfo *test;
  enum DeepState_TestRunResult result;
  const char *reason;
};

/* Pointer to the last registered `TestInfo` structure. */
extern struct DeepState_TestInfo *DeepState_LastTestInfo;

extern int DeepState_TakeOver(void);

/* Defines the entrypoint of a test case. This creates a data structure that
 * contains the information about the test, and then creates an initializer
 * function that runs before `main` that registers the test entrypoint with
 * DeepState. */
#define _DeepState_EntryPoint(test_name, file, line) \
    static void DeepState_Test_ ## test_name (void); \
    static void DeepState_Run_ ## test_name (void) { \
      DeepState_Test_ ## test_name(); \
      DeepState_Pass(); \
    } \
    static struct DeepState_TestInfo DeepState_Info_ ## test_name = { \
      NULL, \
      DeepState_Run_ ## test_name, \
      DEEPSTATE_TO_STR(test_name), \
      file, \
      line, \
    }; \
    DEEPSTATE_INITIALIZER(DeepState_Register_ ## test_name) { \
      DeepState_Info_ ## test_name.prev = DeepState_LastTestInfo; \
      DeepState_LastTestInfo = &(DeepState_Info_ ## test_name); \
    } \
    void DeepState_Test_ ## test_name(void)

/* Set up DeepState. */
extern void DeepState_Setup(void);

/* Tear down DeepState. */
extern void DeepState_Teardown(void);

/* Notify that we're about to begin a test while running under Dr. Fuzz. */
extern void DeepState_BeginDrFuzz(struct DeepState_TestInfo *info);

/* Notify that we're about to begin a test. */
extern void DeepState_Begin(struct DeepState_TestInfo *info);

/* Return the first test case to run. */
extern struct DeepState_TestInfo *DeepState_FirstTest(void);

/* Returns `true` if a failure was caught for the current test case. */
extern bool DeepState_CatchFail(void);

/* Returns `true` if the current test case was abandoned. */
extern bool DeepState_CatchAbandoned(void);

/* Save a passing test to the output test directory. */
extern void DeepState_SavePassingTest(void);

/* Save a failing test to the output test directory. */
extern void DeepState_SaveFailingTest(void);

/* Save a crashing test to the output test directory. */
extern void DeepState_SaveCrashingTest(void);

/* Jump buffer for returning to `DeepState_Run`. */
extern jmp_buf DeepState_ReturnToRun;

/* Checks a filename to see if might be a saved test case.
 *
 * Valid saved test cases have the suffix `.pass` or `.fail`. */
static bool DeepState_IsTestCaseFile(const char *name) {
  const char *suffix = strchr(name, '.');
  if (suffix == NULL) {
    return false;
  }

  const char *extensions[] = {
    ".pass",
    ".fail",
    ".crash",
  };
  const size_t ext_count = sizeof(extensions) / sizeof(char *);

  for (size_t i = 0; i < ext_count; i++) {
    if (!strcmp(suffix, extensions[i])) {
      return true;
    }
  }

  return false;
}

/* Resets the global `DeepState_Input` buffer, then fills it with the
 * data found in the file `path`. */
static void DeepState_InitInputFromFile(const char *path) {
  struct stat stat_buf;

  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    /* TODO(joe): Add error log with more info. */
    DeepState_Abandon("Unable to open file");
  }

  int fd = fileno(fp);
  if (fd < 0) {
    DeepState_Abandon("Tried to get file descriptor for invalid stream");
  }
  if (fstat(fd, &stat_buf) < 0) {
    DeepState_Abandon("Unable to access input file");
  };

  if (stat_buf.st_size > sizeof(DeepState_Input)) {
    /* TODO(joe): Add error log with more info. */
    DeepState_Abandon("File too large");
  }

  /* Reset the input buffer and reset the index. */
  memset((void *) DeepState_Input, 0, sizeof(DeepState_Input));
  DeepState_InputIndex = 0;

  size_t count = fread((void *) DeepState_Input, 1, stat_buf.st_size, fp);
  fclose(fp);

  if (count != stat_buf.st_size) {
    /* TODO(joe): Add error log with more info. */
    DeepState_Abandon("Error reading file");
  }

  DeepState_LogFormat(DeepState_LogInfo,
                      "Initialized test input buffer with data from `%s`",
                      path);
}

/* Run a test case, assuming we have forked from the test harness to do so.
 *
 * An exit code of 0 indicates that the test passed. Any other exit
 * code, or termination by a signal, indicates a test failure. */
static void DeepState_RunTest(struct DeepState_TestInfo *test) {
  /* Run the test. */
  if (!setjmp(DeepState_ReturnToRun)) {
    /* Convert uncaught C++ exceptions into a test failure. */
#if defined(__cplusplus) && defined(__cpp_exceptions)
    try {
#endif  /* __cplusplus */

      test->test_func();  /* Run the test function. */
      exit(DeepState_TestRunPass);

#if defined(__cplusplus) && defined(__cpp_exceptions)
    } catch(...) {
      DeepState_Fail();
    }
#endif  /* __cplusplus */

    /* We caught a failure when running the test. */
  } else if (DeepState_CatchFail()) {
    DeepState_LogFormat(DeepState_LogError, "Failed: %s", test->test_name);
    if (HAS_FLAG_output_test_dir) {
      DeepState_SaveFailingTest();
    }
    exit(DeepState_TestRunFail);

    /* The test was abandoned. We may have gotten soft failures before
     * abandoning, so we prefer to catch those first. */
  } else if (DeepState_CatchAbandoned()) {
    DeepState_LogFormat(DeepState_LogError, "Abandoned: %s", test->test_name);
    exit(DeepState_TestRunAbandon);

    /* The test passed. */
  } else {
    DeepState_LogFormat(DeepState_LogInfo, "Passed: %s", test->test_name);
    if (HAS_FLAG_output_test_dir) {
      DeepState_SavePassingTest();
    }
    exit(DeepState_TestRunPass);
  }
}

/* Fork and run `test`. */
static enum DeepState_TestRunResult
DeepState_ForkAndRunTest(struct DeepState_TestInfo *test) {
  pid_t test_pid = fork();
  if (!test_pid) {
    DeepState_RunTest(test);
  }
  int wstatus;
  waitpid(test_pid, &wstatus, 0);

  /* If we exited normally, the status code tells us if the test passed. */
  if (WIFEXITED(wstatus)) {
    uint8_t status = WEXITSTATUS(wstatus);
    return (enum DeepState_TestRunResult) status;
  }

  /* If here, we exited abnormally but didn't catch it in the signal
   * handler, and thus the test failed due to a crash. */
  return DeepState_TestRunCrash;
}

/* Run a single saved test case with input initialized from the file
 * `name` in directory `dir`. */
static enum DeepState_TestRunResult
DeepState_RunSavedTestCase(struct DeepState_TestInfo *test, const char *dir,
                           const char *name) {
  size_t path_len = 2 + sizeof(char) * (strlen(dir) + strlen(name));
  char *path = (char *) malloc(path_len);
  if (path == NULL) {
    DeepState_Abandon("Error allocating memory");
  }
  if (strncmp(dir, "", strlen(dir)) != 0) {
    snprintf(path, path_len, "%s/%s", dir, name);
  } else {
    snprintf(path, path_len, "%s", name);
  }

  DeepState_InitInputFromFile(path);

  free(path);

  DeepState_Begin(test);

  enum DeepState_TestRunResult result = DeepState_ForkAndRunTest(test);

  if (result == DeepState_TestRunCrash) {
    DeepState_LogFormat(DeepState_LogError, "Crashed: %s", test->test_name);

    if (HAS_FLAG_output_test_dir) {
      DeepState_SaveCrashingTest();
    }

    DeepState_Crash();
  }

  return result;
}

/* Run a single test many times, initialized against each saved test case in
 * `FLAGS_input_test_dir`. */
static int DeepState_RunSavedCasesForTest(struct DeepState_TestInfo *test) {
  int num_failed_tests = 0;
  const char *test_file_name = basename((char *) test->file_name);

  size_t test_case_dir_len = 3 + strlen(FLAGS_input_test_dir)
    + strlen(test_file_name) + strlen(test->test_name);
  char *test_case_dir = (char *) malloc(test_case_dir_len);
  if (test_case_dir == NULL) {
    DeepState_Abandon("Error allocating memory");
  }
  snprintf(test_case_dir, test_case_dir_len, "%s/%s/%s",
           FLAGS_input_test_dir, test_file_name, test->test_name);

  struct dirent *dp;
  DIR *dir_fd;

  dir_fd = opendir(test_case_dir);
  if (dir_fd == NULL) {
    DeepState_LogFormat(DeepState_LogInfo,
                        "Skipping test `%s`, no saved test cases",
                        test->test_name);
    free(test_case_dir);
    return 0;
  }

  /* Read generated test cases and run a test for each file found. */
  while ((dp = readdir(dir_fd)) != NULL) {
    if (DeepState_IsTestCaseFile(dp->d_name)) {
      enum DeepState_TestRunResult result =
        DeepState_RunSavedTestCase(test, test_case_dir, dp->d_name);

      if (result != DeepState_TestRunPass) {
        num_failed_tests++;
      }
    }
  }
  closedir(dir_fd);
  free(test_case_dir);

  return num_failed_tests;
}

/* Run test from `FLAGS_input_test_file`, under `FLAGS_input_which_test`
 * or first test, if not defined. */
static int DeepState_RunSingleSavedTestCase(void) {
  int num_failed_tests = 0;
  struct DeepState_TestInfo *test = NULL;

  DeepState_Setup();

  for (test = DeepState_FirstTest(); test != NULL; test = test->prev) {
    if (HAS_FLAG_input_which_test) {
      if (strncmp(FLAGS_input_which_test, test->test_name, strlen(FLAGS_input_which_test)) == 0) {
	break;
      }
    } else {
      DeepState_LogFormat(DeepState_LogInfo,
			  "No test specified, defaulting to first test");
      break;
    }
  }

  if (test == NULL) {
    DeepState_LogFormat(DeepState_LogInfo,
                        "Could not find matching test for %s",
                        FLAGS_input_which_test);
    return 0;
  }

  enum DeepState_TestRunResult result =
    DeepState_RunSavedTestCase(test, "", FLAGS_input_test_file);

  if (result != DeepState_TestRunPass) {
    if (FLAGS_abort_on_fail) {
      abort();
    }
    num_failed_tests++;
  }

  DeepState_Teardown();

  return num_failed_tests;
}

/* Run tests from `FLAGS_input_test_files_dir`, under `FLAGS_input_which_test`
 * or first test, if not defined. */
static int DeepState_RunSingleSavedTestDir(void) {
  int num_failed_tests = 0;
  struct DeepState_TestInfo *test = NULL;  

  DeepState_Setup();

  for (test = DeepState_FirstTest(); test != NULL; test = test->prev) {
    if (HAS_FLAG_input_which_test) {
      if (strncmp(FLAGS_input_which_test, test->test_name, strlen(FLAGS_input_which_test)) == 0) {
	break;
      }
    } else {
      DeepState_LogFormat(DeepState_LogInfo,
			  "No test specified, defaulting to first test");
      break;
    }
  }

  if (test == NULL) {
    DeepState_LogFormat(DeepState_LogInfo,
                        "Could not find matching test for %s",
                        FLAGS_input_which_test);
    return 0;
  }

  struct dirent *dp;
  DIR *dir_fd;

  struct stat path_stat;

  dir_fd = opendir(FLAGS_input_test_files_dir);
  if (dir_fd == NULL) {
    DeepState_LogFormat(DeepState_LogInfo,
                        "No tests to run.");
    return 0;
  }

  /* Read generated test cases and run a test for each file found. */
  while ((dp = readdir(dir_fd)) != NULL) {
    size_t path_len = 2 + sizeof(char) * (strlen(FLAGS_input_test_files_dir) + strlen(dp->d_name));
    char *path = (char *) malloc(path_len);
    snprintf(path, path_len, "%s/%s", FLAGS_input_test_files_dir, dp->d_name);    
    stat(path, &path_stat);
    
    if (S_ISREG(path_stat.st_mode)) {
      enum DeepState_TestRunResult result =
        DeepState_RunSavedTestCase(test, FLAGS_input_test_files_dir, dp->d_name);

      if (result != DeepState_TestRunPass) {
	if (FLAGS_abort_on_fail) {
	  abort();
	}
	
        num_failed_tests++;
      }
    }
  }
  closedir(dir_fd);

  return num_failed_tests;
}

/* Run test `FLAGS_input_which_test` with saved input from `FLAGS_input_test_file`.
 *
 * For each test unit and case, see if there are input files in the
 * expected directories. If so, use them to initialize
 * `DeepState_Input`, then run the test. If not, skip the test. */
static int DeepState_RunSavedTestCases(void) {
  int num_failed_tests = 0;
  struct DeepState_TestInfo *test = NULL;

  DeepState_Setup();

  for (test = DeepState_FirstTest(); test != NULL; test = test->prev) {
    num_failed_tests += DeepState_RunSavedCasesForTest(test);
  }

  DeepState_Teardown();

  return num_failed_tests;
}

/* Start DeepState and run the tests. Returns the number of failed tests. */
static int DeepState_Run(void) {
  if (!DeepState_OptionsAreInitialized) {
    DeepState_Abandon("Please call DeepState_InitOptions(argc, argv) in main.");
  }

  if (HAS_FLAG_input_test_dir) {
    return DeepState_RunSavedTestCases();
  }

  if (HAS_FLAG_input_test_file) {
    return DeepState_RunSingleSavedTestCase();
  }

  if (HAS_FLAG_input_test_files_dir) {
    return DeepState_RunSingleSavedTestDir();
  }  

  int num_failed_tests = 0;
  int use_drfuzz = getenv("DYNAMORIO_EXE_PATH") != NULL;
  struct DeepState_TestInfo *test = NULL;

  DeepState_Setup();

  for (test = DeepState_FirstTest(); test != NULL; test = test->prev) {
    if (use_drfuzz) {
      if (!fork()) {
        DeepState_BeginDrFuzz(test);
      } else {
        continue;
      }
    } else {
      DeepState_Begin(test);
    }

    num_failed_tests += DeepState_ForkAndRunTest(test);
  }

  if (use_drfuzz) {
    waitpid(-1, NULL, 0);  /* Wait for all children. */
  }

  DeepState_Teardown();

  return num_failed_tests;
}

DEEPSTATE_END_EXTERN_C

#endif  /* SRC_INCLUDE_DEEPSTATE_DEEPSTATE_H_ */
