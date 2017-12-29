#include <linux/kernel.h>
#include <linux/uaccess.h>

// define structure
struct param_input {
	int interval;
	int cnt;
	char start[4];
	char result[4];
};

asmlinkage long sys_pack_params(struct param_input *inputs) {
	struct param_input my_st;
	char ret[4];
	int i;
	
	//printk("[kernel] syscall starts!\n");
	// copy input data from user space
	copy_from_user(&my_st, inputs, sizeof(my_st));
	// using START field of the given input structure, store the result that will be returned to user
	for(i = 0; i < 4; i++) {
		if(my_st.start[i] != '0')	break;
	}
	ret[0] = i;		// starting position
	ret[1] = (my_st.start[i])-'0';	// starting value
	ret[2] = my_st.interval;	// time interval
	ret[3] = my_st.cnt;		// the number of timer handler calls
	for(i = 0; i < 4; i++) {
		my_st.result[i] = ret[i];
	}
	// copy the result back to user space
	copy_to_user(inputs, &my_st, sizeof(my_st));
	return 377;
}
