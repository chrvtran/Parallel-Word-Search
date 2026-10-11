AI Usage Appendix

Students shall (per GAIA)
1. Give credit to AI tools whenever used, even if only to generate ideas rather than usable text or
illustrations.
2. When using AI tools on assignments, add an appendix showing (a) the entire exchange, highlighting
the most relevant sections; (b) a description of precisely which AI tools were used (e.g. ChatGPT
private subscription version or Gemini free version), (c) an explanation of how the AI tools were used
(e.g. to generate ideas, turns of phrase, elements of text, long stretches of text, lines of argument, pieces
of evidence, maps of conceptual territory, illustrations of key concepts, etc.); (d) an account of why AI
tools were used (e.g. to save time, to surmount writer’s block, to stimulate thinking, to handle mounting
stress, to clarify prose, to translate text, to experiment for fun, etc.).
3. Employ AI detection tools and originality checks prior to submission, ensuring that their submitted
work is not mistakenly flagged.
4. Use AI tools wisely and intelligently, aiming to deepen understanding of subject matter and to support
learning.

----

ChatGPT (free default version)

/engine-parallel.c functions search_instance() and instance_worker()

I was really stuck on how to get started with using threading in my search_instance function, and I wanted an answer to quickly move on. I'm looking for ideas on what the best approach is.

Me: (pasting my function) I have the file open. I'm considering the provided #define MAX_LINE_LENGTH 256 (but i think this clipped my console print from earlier?). how should i determine thread size and splitting
ChatGPT: Use fseek() and ftell() to determine the file size in bytes. Don't use MAX_LINE_LENGTH for this; that limits how much text you read at once, not the size of the file.

FILE *file = fopen(filename, "r");

if (file == NULL) {
    return result;
}

fseek(file, 0, SEEK_END);
long file_size = ftell(file);
rewind(file);

Here's what each line does:
- fseek(file, 0, SEEK_END) moves to the end of the file.
- ftell(file) gets the current position, which gives you the file size in bytes.
- rewind(file) moves back to the beginning.
Check for errors in production code: fseek() can fail, and ftell() returns -1L on failure.
Why this works for threading: If the file is 1,000 bytes and you use 2 threads, you can initially divide it into ranges of 500 bytes each. You'll still need to adjust the boundaries so a line isn't split between workers.
And yes, MAX_LINE_LENGTH 256 can cause truncated or split lines if a line exceeds 255 characters. Keep using getline() in your workers so long lines are read in full.
Me: so how do i do the boundaries
ChatGPT: The idea is to divide the file into byte ranges, then make sure each thread starts and ends at a line boundary.
Suppose your file is 1,000 bytes and you have 2 threads:
- Thread 1: bytes 0–499
- Thread 2: bytes 500–999
But byte 500 might be in the middle of a line. So thread 2 needs to skip forward to the next newline before searching.
1. Calculate the initial boundaries
Inside search_instance():
#define NUM_THREADS 2

long chunk_size = file_size / NUM_THREADS;

for (int i = 0; i < NUM_THREADS; i++) {
    args[i].filename = filename;
    args[i].target = target;
    args[i].start = i * chunk_size;
    args[i].end = (i == NUM_THREADS - 1)
                    ? file_size
                    : (i + 1) * chunk_size;
}

This assigns each thread a byte range. The last thread gets any leftover bytes.
2. Adjust the boundaries inside instance_worker()
After seeking to the thread's starting position:
fseek(file, args->start, SEEK_SET);

if (args->start > 0) {
    int ch;

    // Skip the partial line at the start of this chunk
    while ((ch = fgetc(file)) != '\n' && ch != EOF) {
        // Keep reading until the newline
    }
}

Why? If the thread starts halfway through a line, it skips the rest of that line so it doesn't process a line that belongs to the previous chunk.
Then use getline() to read complete lines.
3. Stop at the right boundary
Inside your reading loop, check the position before processing each line:
while (1) {
    long line_start = ftell(file);

    if (line_start >= args->end) {
        break;
    }

    if (getline(&line, &length, file) == -1) {
        break;
    }

    // Your existing strstr() search logic goes here
}

This ensures a thread only starts processing lines whose starting positions fall within its assigned range. A line that crosses the end boundary is processed by the thread that started reading it.
One important detail: use ftell() before getline(), as shown, to record where each line starts.
That gives you the basic boundary-handling strategy without using MAX_LINE_LENGTH or truncating long lines.