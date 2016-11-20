// implementation of matrix multiplication
#include <inc/lib.h>

// topnode - am I a top node? (in which case I don't wait for up node to send result)
// rightnode - am I a right node? (in which case I don't relay value to the right)
void
dataflowNode(int l, int value, envid_t pid_down, envid_t pid_right) {
	for (int i = 0; i < l; ++i) {
		int in_left = ipc_recv(0, 0, 0);
		if (pid_right > 0) ipc_send(pid_right, in_left, 0, 0);

		int in_top = ipc_recv(0,0, 0);
		int result = value * in_left + in_top;
		if (pid_down > 0) ipc_send(pid_down, result, 0, 0);
	}
}

void
multiply(int l, envid_t pids[l][l], int m1[l], int m3[l]) {
	// send each element of m1 into left nodes
	for (int i = 0; i < l; ++i)
		ipc_send(pids[i][0], m1[i], 0, 0);
		
	// send 0 into top row
	for (int i = 0; i < l; ++i)
		ipc_send(pids[0][i], 0, 0, 0);

	// recieve values
	for (int i = 0; i < l; ++i) {
		envid_t src_pid;
		int result = ipc_recv(&src_pid, 0, 0);
		
		for (int j = 0; !m3[j] || j < l; ++j)
			if (pids[l - 1][j] == src_pid)
				m3[j] = result;
	}
}

int
buildGridFromMatrix(int l, int m1[l][l], int m2[l][l], int m3[l][l]) {
	envid_t pids[l][l];

	// make l x l forked processes for the inner grid
	envid_t parent_pid = sys_getenvid();
	for (int i = l - 1; i >= 0; --i) {
		for (int j = l - 1; j >= 0; --j) {
			pids[i][j] = fork();
			if (pids[i][j] < 0)
				panic("matrix: buildGridFromMatrix child %d %d failed to fork with error code %d", i, i, pids[i][j]);
			else if (pids[i][j] == 0) {
				envid_t p_down = (i != l - 1) ? pids[i + 1][j] : parent_pid;
				envid_t p_right = (j != l - 1) ? pids[i][j + 1] : -1;
				dataflowNode(l, m2[i][j], p_down, p_right);
				return 0;
			}
		}
	}

	for (int i = 0; i < l; ++i)
		multiply(l, pids, m1[i], m3[i]);

	return 1;
}

void
umain(int argc, char **argv)
{
  int l = 5;
  int m1[5][5] = {{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}, {6, 7, 8, 9, 10}, {1, 2, 3, 4, 5}};
  int m2[5][5] = {{11, 12, 13, 14, 15}, {6, 7, 8, 9, 10}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}};
  int m3[l][l];

  if (buildGridFromMatrix(l, m1, m2, m3)) {
	  cprintf("\n");
	  for (int i = 0; i < l; ++i)
	  {
	  	for (int j = 0; j < l; ++j)
	  	  cprintf("%3d ", m1[i][j]);
	  	cprintf("\t");
	  	for (int j = 0; j < l; ++j)
	  	  cprintf("%3d ", m2[i][j]);
	  	cprintf("\t\t");
	  	for (int j = 0; j < l; ++j)
	  		cprintf("%3d ", m3[i][j]);
	  	cprintf("\n");
	  }
	  cprintf("\n");
  }
}
