// https://mariusbancila.ro/blog/2020/06/22/a-cpp20-coroutine-example/

#include <time.h>
#include <stdio.h>
#include <stc/cstr.h>
#include <stc/coroutine.h>

struct next_value {
    int val;
    cco_timer tm;
    cco_base base;
};

int next_value(struct next_value* co) {
    cco_async (co) {
        while (true) {
            cco_await_timer(&co->tm, 1 + rand() % 2);
            co->val = rand();
            cco_yield;
        }
    }
    return 0;
}

void print_time(void) {
    time_t now = time(NULL);
    char mbstr[64];
    strftime(mbstr, sizeof(mbstr), "[%H:%M:%S]", localtime(&now));
    printf("%s ", mbstr);
}

// PRODUCER
struct produce {
    struct next_value next;
    cstr text;
    cco_base base;
};

int produce(struct produce* pr) {
    cco_async (pr) {
        pr->text = cstr_init();
        while (true) {
            cco_await_coroutine(next_value(&pr->next), CCO_YIELD);
            cstr_printf(&pr->text, "item %d", pr->next.val);
            print_time();
            printf("produced %s\n", cstr_str(&pr->text));
            cco_yield;
        }

        cco_drop:
        cstr_drop(&pr->text);
        puts("done producer");
    }
    return 0;
}

// CONSUMER
struct consume {
    int n, i;
    cco_base base;
};

int consume(struct consume* co, struct produce* pr) {
   cco_async (co) {
        for (co->i = 1; co->i <= co->n; ++co->i) {
            printf("consumer #%d\n", co->i);
            cco_await_coroutine(produce(pr), CCO_YIELD);
            print_time();
            printf("consumed %s\n", cstr_str(&pr->text));
        }

        cco_drop:
        puts("done consumer");
    }
    return 0;
}

int main(void) {
    struct produce producer = {0};
    struct consume consumer = {.n=5};
    int count = 0;

    cco_run_coroutine(consume(&consumer, &producer)) {
        ++count;
        //cco_sleep(0.001);
        //if (consumer.i == 3) cco_stop(&consumer);
    }

    cco_stop(&producer);
    produce(&producer);
    printf("count: %d\n", count);
}
