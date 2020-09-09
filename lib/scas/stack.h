#ifndef STACK_H
#define STACK_H

typedef struct {
	int capacity;
	int length;
	void **items;
} stack_type;

stack_type *create_stack();
void stack_free(stack_type *stack);
void stack_push(stack_type *stack, void *item);
void *stack_pop(stack_type *stack);
void *stack_peek(stack_type *stack);
void stack_shrink_to_fit(stack_type *stack);

#define STACK_GROWTH_RATE 16

#endif
