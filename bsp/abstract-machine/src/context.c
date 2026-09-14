#include <am.h>
#include <klib.h>
#include <rtthread.h>

rt_thread_t current;

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
    rt_ubase_t *tmp = (rt_ubase_t *)current->user_data;
    rt_ubase_t to = tmp[0];
    rt_ubase_t from = tmp[1];
    if(from)
      *(Context **)from= c;
    c = *(Context **)to;
    break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

static void package(void *args)
{
  void **arg = (void **)args;

  void (*entry)(void *parameter) = (void (*)(void *))arg[0];
  void *parameter = (void *)arg[1];
  void (*exit)(void) = (void (*)(void))arg[2];

  entry(parameter);
  if (exit) exit();
}
/*
static void package(void *args)
{
  void **args_tmp = (void **)args;
  ((void (*)(void *))args_tmp[0])(args_tmp[1]);
  ((void (*)(void))args_tmp[2])();
}*/



void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  current = rt_thread_self();

  rt_ubase_t user_data[2] = {to, 0};
  rt_ubase_t tmp_user_data;

  tmp_user_data = current->user_data;
  current->user_data = (rt_ubase_t)user_data;
  yield();
  current->user_data = tmp_user_data;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_ubase_t user_data[2];
  rt_ubase_t tmp_user_data;

  user_data[0] = to;
  user_data[1] = from;
  current = rt_thread_self();
  tmp_user_data = current->user_data;
  current->user_data = (rt_ubase_t)user_data;
  yield();
  current->user_data = tmp_user_data;
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  //Area stack_ = {.end = (rt_uint8_t *)stack_addr};  
  stack_addr = (rt_uint8_t *)((uintptr_t)stack_addr & ~(sizeof(uintptr_t) - 1));
  Area stack_ = {.end = (rt_uint8_t *)stack_addr}; 
  void **args = (void **)((Context *)stack_addr - 4);
  args[0] = (void *)tentry;
  args[1] = (void *)parameter;
  args[2] = (void *)texit;

  Context *rt = kcontext(stack_, package, args);

  return (rt_uint8_t *)rt;
}
