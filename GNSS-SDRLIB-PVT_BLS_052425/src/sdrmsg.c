//----------------------------------------------------------------------------
//  Real time message queue demo using mq and threads
//
//  To compile: gcc -o mqDemo mqDemo.c -lrt -lpthread
//  To run: ./mqDemo
//  To exit: enter 'q'
//
//  Libs:
//      -lrt        POSIX runtime extensions
//      -lpthread   POSIX threads
//
//  Note: Lots of print statements are added such that user can visualize
//  what is happening
//----------------------------------------------------------------------------

// Include common.h
#include "sdr.h"

//----------------------------------------------------------------------------
// The server pulls (receives) messages from the queue and sends them to the
// terminal (via printf)
//----------------------------------------------------------------------------
extern void *serverthread(void *arg) {

    mqd_t mq;
    struct mq_attr attr;
    char buffer[MAX_SIZE + 1];

    // Initialize the queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    // Select the mq_open flags to use. Include O_NONBLOCK so that
    // mq_receive doesn't just sit there and wait for a message, but
    // rather looks and then continues on.
    int open_flags = 0;
    open_flags = O_CREAT | O_RDONLY | O_NONBLOCK;

    // create the message queue
    mq = mq_open(QUEUE_NAME, open_flags, 0644, &attr);
    CHECK((mqd_t) - 1 != mq);

    // Start server listening
    while (!sdrstat.stopflag) {
        ssize_t bytes_read;

        // receive the message
        bytes_read = mq_receive(mq, buffer,
                                MAX_SIZE, NULL);

        // Can check for bytes with CHECK() if desired
        //CHECK(bytes_read >= 0);

        // If we have data in the buffer, do something
        if (bytes_read > 0) {
            // If we have something in the buffer, add the end-of-line symbols
            buffer[bytes_read] = '\0';

            // Send buffer to terminal
            //printf("Received message at server: %s\n", buffer);
            printf("%s",buffer);

            // Flush the buffer (may not be necessary). Can set the first char
            // to '\0' to designate that it is empty
            buffer[0] = '\0';
        }

    }

    // Server while loop has been terminated, thread is stopping
    printf("Received stopflag, stopping server thread\n");

    // Cleanup
    //CHECK((mqd_t)-1 != mq_close(mq));
    //CHECK((mqd_t)-1 != mq_unlink(QUEUE_NAME));
    mq_close(mq);
    mq_unlink(QUEUE_NAME);

    // Server thread is done
    printf("At server return\n");
    return 0;
}
