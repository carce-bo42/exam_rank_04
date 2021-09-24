#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

# define IN		0
# define OUT 	1

char	**env_ptr;
int		ret = 0;

int	ft_strlen(char *str)
{
	int	i = 0;

	while (str[i])
		i++;
	return (i);
}

void	ft_putstr_fd(int fd, char *str)
{
	write(fd, str, ft_strlen(str));
}

void	print_error_fatal(void)
{
	ft_putstr_fd(2, "error: fatal\n");
	exit(1);
}

void	execve_error(char *str)
{
	ft_putstr_fd(2, "error: cannot execute ");
	ft_putstr_fd(2, str);
	ft_putstr_fd(2, "\n");
	exit(1);
}

void	num_arg_error()
{
	ft_putstr_fd(2, "error: cd: bad arguments\n");
	ret = 1;
}

void	arg_error(char *arg)
{
	ft_putstr_fd(2, "error: cd: cannot change directory to ");
	ft_putstr_fd(2, arg);
	ft_putstr_fd(2, "\n");
	ret = 1;
}

void	close_fd(int fd)
{
	if (close(fd) == -1)
		print_error_fatal();
}

void	dup_fd(int fd, int io)
{
	if (io == IN)
	{
		if (dup2(fd, 0) == -1)
			print_error_fatal();
	}
	else
	{
		if (dup2(fd, 1) == -1)
			print_error_fatal();
	}
}

void	ft_cd(char **av)
{
	if (av[2])
		num_arg_error();
	else
	{
		if (chdir(av[1]) == -1)
		{
			arg_error(av[1]);
			return ;
		}
	}
	ret = 0;
	return ;
}

int	count_processes(char **av)
{
	int	i = 0;
	int	processes = 1;

	while (av[i])
	{
		if (!strcmp(av[i], "|"))
			processes++;
		i++;
	}
	return (processes);
}

void	call_execve(char **av, int n_proc)
{
	pid_t	pid;

	if (n_proc == 1)
	{
		if (!av[0])
			return ;
		if (!strcmp(av[0], "cd"))
		{
			ft_cd(av);
			return ;
		}
		else
		{
			pid = fork();
			if (pid == -1)
				print_error_fatal();
			if (pid == 0)
			{
				if (execve(av[0], av, env_ptr) == -1)
					execve_error(av[0]);
			}
			else
				waitpid(-1, 0, 0);
		}
	}
	else
	{
		if (execve(av[0], av, env_ptr) == -1)
			execve_error(av[0]);
	}
}
 
/* El new_pipe siempre es el que tiene | a su derecha. El old_pipe
 * es el que lo tiene a la izq. El primer y ultimo proceso son asimetricos.
 * NUNCA NUNCA NUNCA se utiliza [1] del old_pipe ni [0] del new_pipe.
 *
 * ESQUEMA PROCESOS:
 *
 *    	  NEW   OLD              NEW   OLD             NEW   OLD          NEW
 *   	X [0] | [1] X          X [0] | [1]           X [0] | [1] X      X [0]
 *  -OUT->[1] | [0]-IN->   -OUT->[1] | [0]-IN->	 -OUT->[1] | [0]-IN->   X [1]
 * */
void	manage_cmd_line(char **av)
{
	int	i = 0;
	int	new_pipe[2];
	int	old_pipe[2];
	int	next_cmd = 0;
	int current_cmd;
	int	n_proc;
	int	current_proc = 0;
	pid_t	pid;

	n_proc = count_processes(av);
	if (n_proc == 1)
		call_execve(av, n_proc);
	else
	{
		while (current_proc++ < n_proc)
		{
			current_cmd = next_cmd;
			i = next_cmd;
			while (av[i])
			{
				if (!strcmp(av[i], "|"))
				{
					*(av[i]) = '\0';
					av[i] = NULL;
					next_cmd = i + 1;
					break ;
				}
				i++;
			}
			if (pipe(new_pipe) == -1)
				print_error_fatal();
			pid = fork();
			if (pid == -1)
				print_error_fatal();
			if (pid == 0)
			{
				close_fd(new_pipe[0]);
				if (current_proc != 1)
					dup_fd(old_pipe[0], IN);
				if (current_proc != n_proc)
					dup_fd(new_pipe[1], OUT);
				else
					close_fd(new_pipe[1]);
				call_execve(av + current_cmd, n_proc);
			}
			else
			{
				close_fd(new_pipe[1]);
				if (current_proc != 1)
					close_fd(old_pipe[0]);
				if (current_proc == n_proc)
					close_fd(new_pipe[0]);
				else
					old_pipe[0] = new_pipe[0];
			}
		}
		while (n_proc-- > 0)
			waitpid(-1, NULL, 0);
	}
}

int	main(int ac, char **av, char **env)
{
	int	i = 0;
	int	cmd_start = 1;

	if (ac > 1)
	{
		env_ptr = env;
		i = 1;
		while (av[i])
		{
			if (!strcmp(av[i], ";"))
			{
				*(av[i]) = '\0';
				av[i] = NULL;
				manage_cmd_line(av + cmd_start);
				cmd_start = i + 1;
			}
			ret = 0;
			i++;
		}
		manage_cmd_line(av + cmd_start);
	}
	return (ret);
}
