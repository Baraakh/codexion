## Guidelines

- Only grade the work that is in the student or group's GiT repository.

- Double-check that the GiT repository belongs to the student or the group. Ensure that the work is for the relevant project and also check that "git clone" is used in an empty folder.

- Check carefully that no malicious aliases was used to fool you and make you evaluate something other than the content of the official repository.

- Remember that for the duration of the defence, no segfault, no other unexpected, premature, uncontrolled or unexpected termination of the program, else the final grade is 0. Use the appropriate flag.
  You should never have to edit any file except the configuration file if it exists.
  If you want to edit a file, take the time to explicit the reasons with the evaluated student and make sure both of you are okay with this.

- You must also verify the absence of memory leaks. Any memory allocated on the heap must be properly freed before the end of execution.
  You are allowed to use any of the different tools available on the computer, such as leaks, valgrind, or e_fence. In case of memory leaks, tick the appropriate flag.

## Preliminaries

### Basics

- This project must be compiling with:
  `-Wall -Wextra -Werror -pthread`
  without any warnings.

- Any compilation error, segmentation fault, data race, deadlock, interleaved log line, or memory leak means 0 to the project.

- The program must take exactly the following arguments, in order:
  `number_of_coders, time_to_burnout, time_to_compile, time_to_debug, time_to_refactor, number_of_compiles_required, dongle_cooldown, scheduler`.

- Each coder must be represented by a thread (using pthreads_create).

- The scheduler must be either "fifo" or "edf".

- On some slow hardware, precise timing may vary slightly; discuss borderline cases honestly before counting them as failures.

- Verify compilation: make

- The binary must be named "codexion".

**[ ] Yes&nbsp;&nbsp;&nbsp;&nbsp;[ ] No**

### Global

- Check if there is any global mutable variable used to manage the shared resources (dongles), the scheduling policy, or the logging.

- If you find such a global state, the evaluation stops here.
  You can go on and check the code, but do not grade the exercises.

**[ ] Yes&nbsp;&nbsp;&nbsp;&nbsp;[ ] No**

### README.md file

Does the repository contain a README.md file at its root, and does it include all of the following?

- The first line is italicized and formatted exactly as: *This project has been created as part of the 42 curriculum by \<login1\>[, \<login2\>, \<login3\>[...]]].*

- A "Description" section explaining the project's purpose and providing a brief overview.

- An "Instructions" section with relevant details about compilation, installation, and/or execution.

- A "Resources" section listing references (documentation, tutorials, etc.) and explaining how AI was used, specifying for which tasks and which parts of the project.

- A "Blocking cases handled" section describing all the concurrency issues addressed in the solution.

- A "Thread synchronization mechanisms" section explaining the specific threading primitives used.

**[ ] Yes&nbsp;&nbsp;&nbsp;&nbsp;[ ] No**

---

## Codexion

### Codexion testing — Easy

- Do not test with more than 200 coders.
- Do not test with timing values under 60ms.
- Verify no coder burns out and all complete required compiles.

**[ ] Yes&nbsp;&nbsp;&nbsp;&nbsp;[ ] No**

### Codexion testing — Less easy

- Validate burnout edge cases.
- Verify correct logging and timing tolerances.
- Check no dongle duplication, correct state transitions.

**[ ] Yes&nbsp;&nbsp;&nbsp;&nbsp;[ ] No**

### Codexion testing — Medium

- Verify cooldown behavior.
- Test scheduler differences (edf vs fifo).
- Validate refactoring timing and log serialization.

**[ ] Yes&nbsp;&nbsp;&nbsp;&nbsp;[ ] No**

### Codexion recode — EDF tie-breaker

- Modify EDF to prefer higher coder_id on equal deadlines. their EDF arbitration.
- Due to the precision of timestamps, equal deadlines may not occur naturally during execution.
- You should verify this behavior by inspecting the code, ensuring that when two deadlines are equal, the higher coder_id is selected.

**[ ] Yes&nbsp;&nbsp;&nbsp;&nbsp;[ ] No**


**[ Finish evaluation ]**
