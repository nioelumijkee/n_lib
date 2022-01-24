#include <unistd.h>
#include <sys/stat.h> // stat
#include <dirent.h>   // DIR opendir
#include <string.h>

#define MAXPATHNAME 2048
#define MAXFILENAME 256
#define MAXFILEINDIR 8192
#define MAXD 64

#define AF_SWAP(BUF, FIRST, SECOND) \
  (BUF) = (FIRST);                  \
  (FIRST) = (SECOND);               \
  (SECOND) = (BUF);

typedef struct _browser
{
  char cdir[MAXPATHNAME];
  short int all;
  short int filter;
  char filename[MAXFILEINDIR][MAXFILENAME];
  short int idx[MAXFILEINDIR];
  short int ext_last[MAXFILEINDIR];
  char mode[MAXFILEINDIR];
  struct stat filestat[MAXFILEINDIR];
} t_browser;

short int f_hidden = 1;
short int f_name = 2;
short int f_mode = 4;
short int f_ex = 8;

//---------------------------------------------------------------------------//
int get_cur_dir(char path[])
{
  if (getcwd(path, MAXPATHNAME) == NULL)
    return 1;
  else
    return 0;
}

//---------------------------------------------------------------------------//
void add_slash(char path[])
{
  int i = 0;
  while (path[i] != '\0')
    i++;
  if (path[i - 1] != '/')
    {
      path[i] = '/';
      path[i + 1] = '\0';
    }
}

//---------------------------------------------------------------------------//
int analyz_path(char path[])
{
  int i = 0;
  int r = -1;
  while (path[i] != '\0')
    {
      if (path[i] == '/')
	{
	  r++;
	}
      i++;
    }
  return r;
}

//---------------------------------------------------------------------------//
void dir_up(char path[])
{
  int i = 0;
  int j = 0;
  int z = 0;
  while (path[i] != '\0')
    {
      if (path[i] == '/')
	{
	  z = j;
	  j = i;
	}
      i++;
    }

  path[z + 1] = '\0';
}

//---------------------------------------------------------------------------//
int read_dir(t_browser *b)
{
  int i, k;
  DIR *dp;
  struct dirent *dirp;
  char buf[MAXPATHNAME];
  
  // open directory
  if ((dp = opendir(b->cdir)) == NULL)
    {
      return 1;
    }
  
  // read directory
  b->all = 0;
  if (b->filter & f_hidden)
    {
      while ((dirp = readdir(dp)) != NULL)
	{
	  // hidden yes
	  if ( ! ( (dirp->d_name[0] == '.' && dirp->d_name[1] == '\0') ||
		   (dirp->d_name[0] == '.' && dirp->d_name[1] == '.' && dirp->d_name[2] == '\0'))) 
	    {
	      k = -1;
	      i = 0;
	      while (dirp->d_name[i])
		{
		  b->filename[b->all][i] = dirp->d_name[i];
		  if (dirp->d_name[i] == '.')
		    k = i;
		  i++;
		}
	      b->filename[b->all][i] = '\0';
	      if (k > 0)
		b->ext_last[b->all] = k + 1;
	      else
		b->ext_last[b->all] = i;
	      b->idx[b->all] = b->all;
	      strcpy(buf, b->cdir);
	      strcat(buf, dirp->d_name);
	      stat(buf, &b->filestat[b->all]);
	      b->all++;
	    }
	}
    }
  else
    {
      while ((dirp = readdir(dp)) != NULL)
	{
	  // hidden no
	  if (dirp->d_name[0] != '.')
	    {
	      k = -1;
	      i = 0;
	      while (dirp->d_name[i])
		{
		  b->filename[b->all][i] = dirp->d_name[i];
		  if (dirp->d_name[i] == '.')
		    k = i;
		  i++;
		}
	      b->filename[b->all][i] = '\0';
	      if (k > 0)
		b->ext_last[b->all] = k + 1;
	      else
		b->ext_last[b->all] = i;
	      b->idx[b->all] = b->all;
	      strcpy(buf, b->cdir);
	      strcat(buf, dirp->d_name);
	      stat(buf, &b->filestat[b->all]);
	      b->all++;
	    }
	}
    }
  
  //        //
  //  SORT  //
  //        //
  
  int start, end, pos, pos2, i1, i2;
  
  if (b->filter & f_name)
    {
      start = 0;
      end = b->all;
      pos = start;
      while (start < end)
	{
	  end--;
	  // down
	  while (pos < end)
	    {
	      // cmp
	      pos2 = pos + 1;
	      i = 0;
	      while (1)
		{
		  if (b->filename[b->idx[pos]][i] > b->filename[b->idx[pos2]][i])
		    {
		      AF_SWAP(k, b->idx[pos], b->idx[pos2])
			break;
		    }
		  else if (b->filename[b->idx[pos]][i] < b->filename[b->idx[pos2]][i])
		    break;
		  i++;
		}
	      pos++;
	    }
	  start++;
	  pos--;
	  // up
	  while (pos >= start)
	    {
	      // cmp
	      pos2 = pos - 1;
	      i = 0;
	      while (1)
		{
		  if (b->filename[b->idx[pos]][i] < b->filename[b->idx[pos2]][i])
		    {
		      AF_SWAP(k, b->idx[pos], b->idx[pos2])
			break;
		    }
		  else if (b->filename[b->idx[pos]][i] > b->filename[b->idx[pos2]][i])
		    break;
		  i++;
		}
	      pos--;
	    }
	}
    }
  
  if (b->filter & f_ex)
    {
      start = 0;
      end = b->all;
      pos = start;
      while (start < end)
	{
	  end--;
	  // down
	  while (pos < end)
	    {
	      // cmp
	      pos2 = pos + 1;
	      i1 = b->ext_last[b->idx[pos]];
	      i2 = b->ext_last[b->idx[pos2]];
	      while (b->filename[b->idx[pos]][i1] != '\0')
		{
		  if (b->filename[b->idx[pos]][i1] > b->filename[b->idx[pos2]][i2])
		    {
		      AF_SWAP(k, b->idx[pos], b->idx[pos2])
			break;
		    }
		  else if (b->filename[b->idx[pos]][i1] < b->filename[b->idx[pos2]][i2])
		    break;
		  i1++;
		  i2++;
		}
	      pos++;
	    }
	  start++;
	  pos--;
	  // up
	  while (pos >= start)
	    {
	      // cmp
	      pos2 = pos - 1;
	      i1 = b->ext_last[b->idx[pos]];
	      i2 = b->ext_last[b->idx[pos2]];
	      while (b->filename[b->idx[pos]][i1] != '\0')
		{
		  if (b->filename[b->idx[pos]][i1] < b->filename[b->idx[pos2]][i2])
		    {
		      AF_SWAP(k, b->idx[pos], b->idx[pos2])
			break;
		    }
		  else if (b->filename[b->idx[pos]][i1] > b->filename[b->idx[pos2]][i2])
		    break;
		  i1++;
		  i2++;
		}
	      pos--;
	    }
	}
    }
  
  if (b->filter & f_mode)
	{
	  // directory
	  pos = 0;
	  for (i = 0; i < b->all; i++)
	    {
	      if (S_ISDIR(b->filestat[b->idx[i]].st_mode))
		{
		  AF_SWAP(k, b->idx[pos], b->idx[i])
		    pos++;
		}
	    }
	  // symbolic dev
	  for (i = pos; i < b->all; i++)
		{
		  if (S_ISCHR(b->filestat[b->idx[i]].st_mode))
		    {
		      AF_SWAP(k, b->idx[pos], b->idx[i])
			pos++;
		    }
		}
	  // block dev
	  for (i = pos; i < b->all; i++)
	    {
	      if (S_ISBLK(b->filestat[b->idx[i]].st_mode))
		{
		  AF_SWAP(k, b->idx[pos], b->idx[i])
		    pos++;
		}
	    }
	  // fifo
	  for (i = pos; i < b->all; i++)
	    {
	      if (S_ISFIFO(b->filestat[b->idx[i]].st_mode))
		{
		  AF_SWAP(k, b->idx[pos], b->idx[i])
		    pos++;
		}
	    }
	  // link
	  for (i = pos; i < b->all; i++)
	    {
	      if (S_ISLNK(b->filestat[b->idx[i]].st_mode))
		{
		  AF_SWAP(k, b->idx[pos], b->idx[i])
		    pos++;
		}
	    }
	  // socket
	  for (i = pos; i < b->all; i++)
	    {
	      if (S_ISSOCK(b->filestat[b->idx[i]].st_mode))
		{
		  AF_SWAP(k, b->idx[pos], b->idx[i])
		    pos++;
		}
	    }
	}
  
  //        //
  //  MODE  //
  //        //
  for (i = 0; i < b->all; i++)
    {
      if (S_ISDIR(b->filestat[b->idx[i]].st_mode))
	{
	  b->mode[b->idx[i]] = 0;
	}
      else if (S_ISCHR(b->filestat[b->idx[i]].st_mode))
	{
	  b->mode[b->idx[i]] = 1;
	}
      else if (S_ISBLK(b->filestat[b->idx[i]].st_mode))
	{
	  b->mode[b->idx[i]] = 2;
	}
      else if (S_ISFIFO(b->filestat[b->idx[i]].st_mode))
	{
	  b->mode[b->idx[i]] = 3;
	}
      else if (S_ISLNK(b->filestat[b->idx[i]].st_mode))
	{
	  b->mode[b->idx[i]] = 4;
	}
      else if (S_ISSOCK(b->filestat[b->idx[i]].st_mode))
	{
	  b->mode[b->idx[i]] = 5;
	}
      else
	{
	  b->mode[b->idx[i]] = 6;
	}
    }
  
  return 0;
}
