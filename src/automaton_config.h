/** @file
*
*  Automaton config file (C99 standard)
*
*  Each macro definition is documentated.
*  Use them to change behaviour of the application.
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#ifndef __AUTOMATON_CONF_H__
#define __AUTOMATON_CONF_H__


/**
 * @def SERVER_TERMINATE_ON_RUN_FAILURE
 *    If this macro value is equal to 1 then server will imidiatelly terminate itself
 *    when any of the run processes will return non-zero exit code or terminate abnormally.
 *
 *    In all other cases the server will display warning about wroker crash,
 *    but it will conitnue normal work.
 */
#define SERVER_TERMINATE_ON_RUN_FAILURE   0

/**
 * @def RUN_WORKLOAD_LIMIT
 *    Limit of the workload per wroker process in execution of async accept.
 *
 *    This means that fork's will occur only after parsing at least RUN_WORKLOAD_LIMIT nodes.
 *    Higher value means less worker processes.
 */
#define RUN_WORKLOAD_LIMIT      5

/**
 * @def RUN_FORK_LIMIT
 *    Limit of the self-forks done per worker proccess.
 *
 *    It approximated upper limit of self-forks done by worker.
 */
#define RUN_FORK_LIMIT          22

/**
 * @def SERVER_PROCESS_LIMIT
 *    Limit of the number of processes per server.
 *
 *    The server tries to throttle the number of spawned processes, so that number of processes after
 *    reaching RUN_WORKLOAD_LIMIT is constant or rises by negglible amounts randomly.
 *    (this value is only an estimate not exact maximum value of launched processes)
 *
 *    Use this setting to limit estimated maximum amount of fork's done by server.
 */
#define SERVER_PROCESS_LIMIT    20

/**
 * @def MAX_Q
 *    Defines maximum number of automaton states
 */
#define MAX_Q                  107

/**
 * @def MAX_A
 *    Defines maximum number of alphabet characters
 */
#define MAX_A                  37

/**
 * @def LINE_BUF_SIZE
 *    Defines maximum number of characters in single line
 */
#define LINE_BUF_SIZE          1020

/**
 * @def FILE_BUF_SIZE
 *    Defines maximum number of bytes in single file
 */
#define FILE_BUF_SIZE          3000007

/**
 * @def MSG_QUEUE_SIZE
 *    Defines maximum number of messages in a msgQueue
 */
#define MSG_QUEUE_SIZE         10

/**
 * @def USE_ASYNC_ACCEPT
 *    If set to 1 then async accept function will be used.
 *    If set to 0 then synchronic version will be used and no run subprocess will be spawned.
 */
#define USE_ASYNC_ACCEPT        1

/**
 * @def DEBUG_TRANSFERRED_GRAPH
 *    If set to 1 then transition graph is printed in each run.
 *    Use it to make sure the correct graph is beeing transmitted to the worker.
 *    Logging must be also enabled to see the results.
 */
#define DEBUG_TRANSFERRED_GRAPH 0

/**
 * @def DEBUG_ACCEPT_RUN
 *    If set to 1 then accept will print each node it parses.
 *    Useful when debugging accept function.
 *    Logging must be also enabled to see the results.
 */
#define DEBUG_ACCEPT_RUN        0

/**
 * @def SERVER_FORK_RETRY_COUNT
 *   It the server has failed to fork the worker it will retry to do the fork.
 *   The maximum number of such re-forks is set up by this macro value.
 *
 */
#define SERVER_FORK_RETRY_COUNT 3

#endif // __AUTOMATON_CONF_H__