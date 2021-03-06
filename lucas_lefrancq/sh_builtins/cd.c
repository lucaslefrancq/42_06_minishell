/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd.c                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lucas <lucas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/10/08 15:34:05 by lucas          #+#    #+#             */
/*   Updated: 2020/10/26 18:14:43 by lucas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

int		len_var_name(char *var);

/*
** Returns the index of var in environnement array, or the index of the NULL
** terminated if var doesn't exist in env.
*/
int		find_var_in_env(char *var, char **env)
{
	int		i;

	i = 0;
	while (env[i] && (ft_strncmp(var, env[i], ft_strlen(var)) || 
		len_var_name(var) != len_var_name(env[i]))) //until we match an existing variable
		i++;
	return (i);
}

/*
** Prints an error message with either path or $HOME, frees path and returns 1.
*/
int		error_no_file(char *arg, char **env, char *path, char *save_path, char *cmd)
{
	int		i;

	i = 0;
	if (arg)
	{
		arg[0] == '~' && save_path ? arg = save_path : 0;
		ft_fd_printf(STDERR_FILENO, "minishell: %s: %s: No such file or directory\n", cmd, arg);
	}
	if (!arg)
	{
		i = find_var_in_env("HOME", env);
		ft_fd_printf(STDERR_FILENO, "minishell: %s: %s: No such file or directory\n", cmd, env[i] + 5);
	}
	free(path);
	free(save_path);
	return (FAILURE);
}

/*
** Prints an error message with either path or $HOME, frees path and returns 1.
*/
int		error_not_dir(char *arg, char **env, char *path, char *save_path, char *cmd)
{
	int		i;

	i = 0;
	if (arg)
	{
		arg[0] == '~' && save_path ? arg = save_path : 0;
		ft_fd_printf(STDERR_FILENO, "minishell: %s: %s: Not a directory\n", cmd, arg);
	}
	if (!arg)
	{
		i = find_var_in_env("HOME", env);
		ft_fd_printf(STDERR_FILENO, "minishell: %s: %s: Not a directory\n", cmd, env[i] + 5);
	}
	free(path);
	free(save_path);
	return (FAILURE);
}

/*
** Prints an error message, frees path and returns 1.
*/
int		cd_error_need_free(char *str_error, char *path)
{
	ft_fd_printf(STDERR_FILENO, "%s", str_error);
	free(path);
	return (FAILURE);
}

/*
** Searches for HOME environnement variable and returns a *char with only the
** path. If HOME isn't set, returns NULL.
*/
char	*copy_home_var(char **env, char *cmd)
{
	int		i;
	char	*tmp;

	i = 0;
	i = find_var_in_env("HOME", env);
	if ((!env[i] || (env[i] && ft_strncmp(env[i], "HOME=", 5)) ||               //HOME variable doesn't exist
			(env[i] && !ft_strncmp(env[i], "HOME=", 5) && env[i][5] == '\0'))   //or HOME=""
			&& ft_fd_printf(STDERR_FILENO, "minishell: %s: HOME not set\n", cmd))
		return (NULL);
	if (!(tmp = ft_strdup(env[i])) && ft_fd_printf(STDERR_FILENO, "minishell: %s: malloc failed\n", cmd))
		return (NULL);
	ft_strlcpy(tmp, tmp + 5, ft_strlen(tmp + 5) + 1); //removing HOME=
	return (tmp);
}

/*
** Transforms the relative path into absolute path.
*/
char	*add_absolute_path_to_relative(char *pwd, char *arg)
{
	char	*tmp;

	tmp = pwd;
	if ((!(pwd[0] == '/' && pwd[1] == '\0') && !(pwd[0] == '/' && pwd[1]  //except for "cd /" or "cd //"
			== '/' && pwd[2] == '\0')) && !(pwd = ft_strjoin(pwd, "/")))
		return (NULL);
	!(pwd[0] == '/' && pwd[1] == '\0') && !(pwd[0] == '/' && pwd[1] == //except for "cd /" or "cd //"
			'/' && pwd[2] == '\0') ? free(tmp) : 0;
	tmp = pwd;
	if (!(pwd = ft_strjoin(pwd, arg)))
		return (NULL);
	free(tmp);
	return (pwd);
}

/*
** Returns a *char on the previous '/', or at the beginning of path if there
** isn't a previous '/'.
*/
int		find_prev_slash(char *path, int i)
{
	while (i > 0 && path[i] != '/')
		i--;
	if (i < 0)
		i = 0;
	return (i);
}

/*
** Ends the path with '/'.
*/
char	*add_end_slash(char *path)
{
	char	*tmp;
	int		i;

	i = 0;
	while (path[i])
		i++;
	if (!(tmp = malloc(i + 2)))
		return (NULL);
	tmp[i + 1] = '\0';
	ft_strlcpy(tmp, path, i + 1);
	tmp[i] = '/';
	free(path);
	path = tmp;
	return (path);
}

/*
** Each time a double dot is removed + the part just before it, try to access
** the path (cut at i position). Returns -1 if the path is incorrect.
*/
int		check_if_correc_path(char *tmp_path, int i)
{
	struct stat	info_file;
	
	i++;
	if (!tmp_path) //malloc failed
		return (FAILURE);
	if (!i) //if i = 0 >> path is '/' so it's correct
	{
		free(tmp_path);
		return (SUCCESS);
	}
	tmp_path[i] = '\0'; //trying new path just after we removed .. and the part before it
	if (stat(tmp_path, &info_file) == -1) //incorrect path
		return (-1);
	free(tmp_path);
	return (SUCCESS);
}

/*
** Removes .. from the path. For each .., removes also the directory before it.
** Checks each time it removes a double dot if the new path is correct. If not,
** returns the incorrect path so stat / chdir will fail when trying to access it.
*/
char	*remove_double_dots(char *path)
{
	int		i;
	int		correct_path;
	int		prev_slash;

	i = -1;
	while (path[++i])
	{
		if (path[i] == '.' && path[i + 1] == '.' && path[i - 1] == '/' &&    //remove '/..' and the part until previous '/'
				(path[i + 2] == '\0' || path[i + 2] == '/'))
		{
			if (path[i + 2] == '\0' && !(path = add_end_slash(path))) //adds a '/' if /.. is at the end of path
				return (NULL);
			prev_slash = find_prev_slash(path, i - 2); // if /tmp/../ > returns the position of '/' before tmp
			ft_strlcpy(&path[prev_slash], &path[i + 2], ft_strlen(&path[i + 2]) + 1);
			i = prev_slash - 1;
			correct_path = check_if_correc_path(ft_strdup(path), i); //new
			if (correct_path == FAILURE) //case malloc failed
				return (NULL);
			else if (correct_path == -1) //path incorrect, we returned it and chdir will fail
				return (path);
		}
	}
	return (path);
}

/*
** Removes . from the path.
*/
char	*remove_simple_dot(char *path)
{
	int		i;

	i = -1;
	while (path[++i])
	{
		if (path[i] == '.' && path[i - 1] == '/' && (path[i + 1] == '\0' || path[i + 1] == '/'))   //remove '/.'
		{
			if (path[i + 1] == '\0' && !(path = add_end_slash(path))) //adds a '/' if /. is at the end of path
				return (NULL);
			ft_strlcpy(&path[i - 1], &path[i + 1], ft_strlen(&path[i + 1]) + 1);
			i--;
		}
	}
	return (path);
}

/*
** Removes all the multiple slash, except for the beginning of the path : it
** be double but double only. More slash will lead to just one slash.
** Ex : //tmp > //tmp, ///tmp > /tmp.
*/
int		remove_multiple_slash(char *path)
{
	int		i;

	if (!path[0]) //case cd ""
		return (SUCCESS);
	i = 0;
	if (path[i] == '/' && path[i + 1] == '/' && path[i + 2] == '/') //if there is at least 3 slashs at beginning of path >> we keep only one
		i = -1;                                                     //otherwise we do not treat first '/' because it can be double (only the first one !)
	while (path[++i])
	{
		if (path[i] == '/' && path[i + 1] == '/')
		{
			ft_strlcpy(&path[i], &path[i + 1], ft_strlen(&path[i + 1]) + 1);
			i--;
		}
	}
	return (SUCCESS);
}

/*
** Transforms path into something that can be used by chdir function. Handles
** '.', '..' and multiple slashs. Also transforms relative path (including ~/)
** into absolute path.
*/
char	*treat_relative_path(char *arg, char **env)
{
	int		i;
	char	*path;

	path = NULL;
	if (arg[0] != '/' && arg[0] != '~') //if relative path
	{

		if (!(path = ft_strdup(global_path)) || !(path = add_absolute_path_to_relative(path, arg)))
			return (error_msg_ptr("cd: malloc failed\n", NULL));
	}
	else if (arg[0] == '~')
	{
		i = find_var_in_env("HOME", env);
		if (env[i]) //updating global_home variable if $HOME exists in environnement
		{
			free(global_home);
			if (!(global_home = ft_strdup(&env[i][5]))) //starting after HOME=
				return (NULL);
		}
		if (!(path = ft_strdup(global_home)))
			return (NULL);
		if (arg[1] != '/' && arg[1] != '\0') //case ~ without / (ex : ~Dir >> error)
		{
			error_no_file(arg, env, path, NULL, "cd");
			return (NULL);
		}
		i = (arg[1] == '/' && path[1] != '\0') ? 1 : 0; //for cd ~/ with $HOME=/
		if (!(path = add_absolute_path_to_relative(path, arg + i + 1))) //joins $HOME value and arg without tilde
			return (error_msg_ptr("cd: malloc failed\n", NULL));
	}
	else if (!(path = ft_strdup(arg))) //if absolute
		return (error_msg_ptr("cd: malloc failed\n", NULL));
	return (path);
}

/*
** Joins var and path to create a new env variable and update env array
** (for $PWD / $OLDPWD).
*/
int		update_env(char *var, char *path, char **env, int i)
{
	char *tmp;

	if (!(tmp = ft_strjoin(var, path)))
		return (FAILURE);
	free(env[i]);
	env[i] = tmp;
	return (SUCCESS);
}

/*
** Updates $PWD and $OLDPWD if they exist.
*/
int		update_env_pwd_oldpwd(char *path, char **env)
{
	int		i;

	i = find_var_in_env("OLDPWD", env);
	if (env[i] && update_env("OLDPWD=", global_path, env, i)) //copying global_path (previous $PWD) in $OLDPWD
		return (FAILURE);	
	i = 0;
	while (path[i])
		i++;
	if (i > 2 && path[i - 1] == '/')    //removing the shlash at the end if it exist
		path[i - 1] = '\0';             //except when path is '/' or '//
	i = find_var_in_env("PWD", env);
	if (env[i] && update_env("PWD=", path, env , i)) //if $HOME var exists we update it
		return (FAILURE);
	return (SUCCESS);
}

/*
** If no arguments, changes the working directory to $HOME if it exists.
** Otherwise changes the working directory using first argument (and ignoring
** the other ones). Handles absolute path, relative path (also with ~).
** Doesn't take any options. Returns 0 if success, 1 if failure.
*/
int		builtin_cd(char **args, char **env)
{
	char	*path;      //path will be send to chdir func
	struct stat	info_file;
	char	*save_path; //for handling ~ error msg with correct path
	
	save_path = NULL;
	if (args && args[1] && args[2])
        return (error_msg("cd: too many arguments\n", FAILURE));
	if (args && args[1] && args[1][0] == '-') //our cd doesn't handle options
        return (error_msg("cd: no options are allowed\n", FAILURE));
	if (args && !args[1]) //if no arg, cd use HOME environnement variable
	{
		if (!(path = copy_home_var(env, "cd")))
			return (FAILURE);
	}
	else if (!(path = treat_relative_path(args[1], env)) || !(save_path = ft_strdup(path))) //we treat the argument for updating $PWD
		return (FAILURE);
	if (stat(path, &info_file) != -1 && !remove_multiple_slash(path)
			&& (!(path = remove_simple_dot(path)) || !(path = remove_double_dots(path)))) //modifying path for updating $PWD
		return (error_msg("cd: malloc failed\n", FAILURE));
	if (stat(path, &info_file) == -1) //checking if file's path / directory's path exists
		return (error_no_file(args[1], env, path, save_path, "cd")); //save_path for correct error msg with ~
	if (chdir(path) == -1) //using path and not args[1] because cd can be use without argument
		return (error_not_dir(args[1], env, path, save_path, "cd"));
	if (update_env_pwd_oldpwd(path, env))
		return (cd_error_need_free("minishell: cd: malloc failed\n", path));
	free(global_path);
	free(save_path);
	global_path = path;
	return (SUCCESS);
}
