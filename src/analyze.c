/* Analyze file differences for DIFFCOUNT.

   DIFF part comes from GNU DIFF
   Copyright (C) 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1998, 2001,
   2002 Free Software Foundation, Inc.

   This file is part of DIFFCOUNT.
   Count Part developed by C.YANG. 2006

   DIFFCOUNT is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation;

   This file is part of HikSvnCount.
   SVN Count Part developed by Haiyuan.Qian 2013
   Software Configuration Management of HANGZHOU HIKVISION DIGITAL TECHNOLOGY CO.,LTD.
   email:qianhaiyuan@hikvision.com

   HikSvnCount is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation;
   */


#include "diffcount.h"
#include <cmpbuf.h>
#include <error.h>
#include <regex.h>
#include <xalloc.h>
#include <stdlib.h>
#include <locale.h>

static lin *xvec, *yvec;	/* Vectors being compared. */
static lin *fdiag;		/* Vector, indexed by diagonal, containing
				   1 + the X coordinate of the point furthest
				   along the given diagonal in the forward
				   search of the edit matrix. */
static lin *bdiag;		/* Vector, indexed by diagonal, containing
				   the X coordinate of the point furthest
				   along the given diagonal in the backward
				   search of the edit matrix. */
static lin too_expensive;	/* Edit scripts longer than this are too
				   expensive to compute.  */

#define SNAKE_LIMIT 20	/* Snakes bigger than this are considered `big'.  */

struct partition
{
  lin xmid, ymid;	/* Midpoints of this partition.  */
  bool lo_minimal;	/* Nonzero if low half will be analyzed minimally.  */
  bool hi_minimal;	/* Likewise for high half.  */
};

/* Find the midpoint of the shortest edit script for a specified
   portion of the two files.

   Scan from the beginnings of the files, and simultaneously from the ends,
   doing a breadth-first search through the space of edit-sequence.
   When the two searches meet, we have found the midpoint of the shortest
   edit sequence.

   If FIND_MINIMAL is nonzero, find the minimal edit script regardless
   of expense.  Otherwise, if the search is too expensive, use
   heuristics to stop the search and report a suboptimal answer.

   Set PART->(xmid,ymid) to the midpoint (XMID,YMID).  The diagonal number
   XMID - YMID equals the number of inserted lines minus the number
   of deleted lines (counting only lines before the midpoint).
   Return the approximate edit cost; this is the total number of
   lines inserted or deleted (counting only lines before the midpoint),
   unless a heuristic is used to terminate the search prematurely.

   Set PART->lo_minimal to true iff the minimal edit script for the
   left half of the partition is known; similarly for PART->hi_minimal.

   This function assumes that the first lines of the specified portions
   of the two files do not match, and likewise that the last lines do not
   match.  The caller must trim matching lines from the beginning and end
   of the portions it is going to specify.

   If we return the "wrong" partitions,
   the worst this can do is cause suboptimal diff output.
   It cannot cause incorrect diff output.  */

static lin diag (lin xoff, lin xlim, lin yoff, lin ylim, bool find_minimal,
      struct partition *part)
{
  lin *const fd = fdiag;	/* Give the compiler a chance. */
  lin *const bd = bdiag;	/* Additional help for the compiler. */
  lin const *const xv = xvec;	/* Still more help for the compiler. */
  lin const *const yv = yvec;	/* And more and more . . . */
  lin const dmin = xoff - ylim;	/* Minimum valid diagonal. */
  lin const dmax = xlim - yoff;	/* Maximum valid diagonal. */
  lin const fmid = xoff - yoff;	/* Center diagonal of top-down search. */
  lin const bmid = xlim - ylim;	/* Center diagonal of bottom-up search. */
  lin fmin = fmid, fmax = fmid;	/* Limits of top-down search. */
  lin bmin = bmid, bmax = bmid;	/* Limits of bottom-up search. */
  lin c;			/* Cost. */
  bool odd = (fmid - bmid) & 1;	/* True if southeast corner is on an odd
				   diagonal with respect to the northwest. */

  fd[fmid] = xoff;
  bd[bmid] = xlim;

  for (c = 1;; ++c)
    {
      lin d;			/* Active diagonal. */
      bool big_snake = 0;

      /* Extend the top-down search by an edit step in each diagonal. */
      fmin > dmin ? fd[--fmin - 1] = -1 : ++fmin;
      fmax < dmax ? fd[++fmax + 1] = -1 : --fmax;
      for (d = fmax; d >= fmin; d -= 2)
	{
	  lin x, y, oldx, tlo = fd[d - 1], thi = fd[d + 1];

	  if (tlo >= thi)
	    x = tlo + 1;
	  else
	    x = thi;
	  oldx = x;
	  y = x - d;
	  while (x < xlim && y < ylim && xv[x] == yv[y])
	    ++x, ++y;
	  if (x - oldx > SNAKE_LIMIT)
	    big_snake = 1;
	  fd[d] = x;
	  if (odd && bmin <= d && d <= bmax && bd[d] <= x)
	    {
	      part->xmid = x;
	      part->ymid = y;
	      part->lo_minimal = part->hi_minimal = 1;
	      return 2 * c - 1;
	    }
	}

      /* Similarly extend the bottom-up search.  */
      bmin > dmin ? bd[--bmin - 1] = LIN_MAX : ++bmin;
      bmax < dmax ? bd[++bmax + 1] = LIN_MAX : --bmax;
      for (d = bmax; d >= bmin; d -= 2)
	{
	  lin x, y, oldx, tlo = bd[d - 1], thi = bd[d + 1];

	  if (tlo < thi)
	    x = tlo;
	  else
	    x = thi - 1;
	  oldx = x;
	  y = x - d;
	  while (x > xoff && y > yoff && xv[x - 1] == yv[y - 1])
	    --x, --y;
	  if (oldx - x > SNAKE_LIMIT)
	    big_snake = 1;
	  bd[d] = x;
	  if (!odd && fmin <= d && d <= fmax && x <= fd[d])
	    {
	      part->xmid = x;
	      part->ymid = y;
	      part->lo_minimal = part->hi_minimal = 1;
	      return 2 * c;
	    }
	}

      if (find_minimal)
	continue;

      /* Heuristic: check occasionally for a diagonal that has made
	 lots of progress compared with the edit distance.
	 If we have any such, find the one that has made the most
	 progress and return it as if it had succeeded.

	 With this heuristic, for files with a constant small density
	 of changes, the algorithm is linear in the file size.  */

      if (200 < c && big_snake )
	{
	  lin best;

	  best = 0;
	  for (d = fmax; d >= fmin; d -= 2)
	    {
	      lin dd = d - fmid;
	      lin x = fd[d];
	      lin y = x - d;
	      lin v = (x - xoff) * 2 - dd;
	      if (v > 12 * (c + (dd < 0 ? -dd : dd)))
		{
		  if (v > best
		      && xoff + SNAKE_LIMIT <= x && x < xlim
		      && yoff + SNAKE_LIMIT <= y && y < ylim)
		    {
		      /* We have a good enough best diagonal;
			 now insist that it end with a significant snake.  */
		      int k;

		      for (k = 1; xv[x - k] == yv[y - k]; k++)
			if (k == SNAKE_LIMIT)
			  {
			    best = v;
			    part->xmid = x;
			    part->ymid = y;
			    break;
			  }
		    }
		}
	    }
	  if (best > 0)
	    {
	      part->lo_minimal = 1;
	      part->hi_minimal = 0;
	      return 2 * c - 1;
	    }

	  best = 0;
	  for (d = bmax; d >= bmin; d -= 2)
	    {
	      lin dd = d - bmid;
	      lin x = bd[d];
	      lin y = x - d;
	      lin v = (xlim - x) * 2 + dd;
	      if (v > 12 * (c + (dd < 0 ? -dd : dd)))
		{
		  if (v > best
		      && xoff < x && x <= xlim - SNAKE_LIMIT
		      && yoff < y && y <= ylim - SNAKE_LIMIT)
		    {
		      /* We have a good enough best diagonal;
			 now insist that it end with a significant snake.  */
		      int k;

		      for (k = 0; xv[x + k] == yv[y + k]; k++)
			if (k == SNAKE_LIMIT - 1)
			  {
			    best = v;
			    part->xmid = x;
			    part->ymid = y;
			    break;
			  }
		    }
		}
	    }
	  if (best > 0)
	    {
	      part->lo_minimal = 0;
	      part->hi_minimal = 1;
	      return 2 * c - 1;
	    }
	}

      /* Heuristic: if we've gone well beyond the call of duty,
	 give up and report halfway between our best results so far.  */
      if (c >= too_expensive)
	{
	  lin fxybest, fxbest;
	  lin bxybest, bxbest;

	  fxbest = bxbest = 0;  /* Pacify `gcc -Wall'.  */

	  /* Find forward diagonal that maximizes X + Y.  */
	  fxybest = -1;
	  for (d = fmax; d >= fmin; d -= 2)
	    {
	      lin x = MIN (fd[d], xlim);
	      lin y = x - d;
	      if (ylim < y)
		x = ylim + d, y = ylim;
	      if (fxybest < x + y)
		{
		  fxybest = x + y;
		  fxbest = x;
		}
	    }

	  /* Find backward diagonal that minimizes X + Y.  */
	  bxybest = LIN_MAX;
	  for (d = bmax; d >= bmin; d -= 2)
	    {
	      lin x = MAX (xoff, bd[d]);
	      lin y = x - d;
	      if (y < yoff)
		x = yoff + d, y = yoff;
	      if (x + y < bxybest)
		{
		  bxybest = x + y;
		  bxbest = x;
		}
	    }

	  /* Use the better of the two diagonals.  */
	  if ((xlim + ylim) - bxybest < fxybest - (xoff + yoff))
	    {
	      part->xmid = fxbest;
	      part->ymid = fxybest - fxbest;
	      part->lo_minimal = 1;
	      part->hi_minimal = 0;
	    }
	  else
	    {
	      part->xmid = bxbest;
	      part->ymid = bxybest - bxbest;
	      part->lo_minimal = 0;
	      part->hi_minimal = 1;
	    }
	  return 2 * c - 1;
	}
    }
}


/* Compare in detail contiguous subsequences of the two files
   which are known, as a whole, to match each other.

   The results are recorded in the vectors files[N].changed, by
   storing 1 in the element for each line that is an insertion or deletion.

   The subsequence of file 0 is [XOFF, XLIM) and likewise for file 1.

   Note that XLIM, YLIM are exclusive bounds.
   All line numbers are origin-0 and discarded lines are not counted.

   If FIND_MINIMAL, find a minimal difference no matter how
   expensive it is.  */

static void compareseq (lin xoff, lin xlim, lin yoff, lin ylim, bool find_minimal)
{
  lin * const xv = xvec; /* Help the compiler.  */
  lin * const yv = yvec;

  /* Slide down the bottom initial diagonal. */
  while (xoff < xlim && yoff < ylim && xv[xoff] == yv[yoff])
    ++xoff, ++yoff;
  /* Slide up the top initial diagonal. */
  while (xlim > xoff && ylim > yoff && xv[xlim - 1] == yv[ylim - 1])
    --xlim, --ylim;

  /* Handle simple cases. */
  if (xoff == xlim)
    while (yoff < ylim)
      files[1].changed[files[1].realindexes[yoff++]] = 1;
  else if (yoff == ylim)
    while (xoff < xlim)
      files[0].changed[files[0].realindexes[xoff++]] = 1;
  else
    {
      lin c;
      struct partition part;

      /* Find a point of correspondence in the middle of the files.  */

      c = diag (xoff, xlim, yoff, ylim, find_minimal, &part);

      if (c == 1)
	{
	  /* This should be impossible, because it implies that
	     one of the two subsequences is empty,
	     and that case was handled above without calling `diag'.
	     Let's verify that this is true.  */
	  abort ();
	}
      else
	{
	  /* Use the partitions to split this problem into subproblems.  */
	  compareseq (xoff, part.xmid, yoff, part.ymid, part.lo_minimal);
	  compareseq (part.xmid, xlim, part.ymid, ylim, part.hi_minimal);
	}
    }
}




/* Discard lines from one file that have no matches in the other file.

   A line which is discarded will not be considered by the actual
   comparison algorithm; it will be as if that line were not in the file.
   The file's `realindexes' table maps virtual line numbers
   (which don't count the discarded lines) into real line numbers;
   this is how the actual comparison algorithm produces results
   that are comprehensible when the discarded lines are counted.

   When we discard a line, we also mark it as a deletion or insertion
   so that it will be printed in the output.  */

static void discard_confusing_lines (struct file_data filevec[])
{
  int f;
  lin i;
  char *discarded[2];
  lin *equiv_count[2];
  lin *p;

  /* Allocate our results.  */
  p = xmalloc ((filevec[0].buffered_lines + filevec[1].buffered_lines)
	       * (2 * sizeof *p));
  for (f = 0; f < 2; f++)
    {
      filevec[f].undiscarded = p;  p += filevec[f].buffered_lines;
      filevec[f].realindexes = p;  p += filevec[f].buffered_lines;
    }

  /* Set up equiv_count[F][I] as the number of lines in file F
     that fall in equivalence class I.  */

  p = zalloc (filevec[0].equiv_max * (2 * sizeof *p));
  equiv_count[0] = p;
  equiv_count[1] = p + filevec[0].equiv_max;

  for (i = 0; i < filevec[0].buffered_lines; ++i)
    ++equiv_count[0][filevec[0].equivs[i]];
  for (i = 0; i < filevec[1].buffered_lines; ++i)
    ++equiv_count[1][filevec[1].equivs[i]];

  /* Set up tables of which lines are going to be discarded.  */

  discarded[0] = zalloc (filevec[0].buffered_lines
			 + filevec[1].buffered_lines);
  discarded[1] = discarded[0] + filevec[0].buffered_lines;

  /* Mark to be discarded each line that matches no line of the other file.
     If a line matches many lines, mark it as provisionally discardable.  */

  for (f = 0; f < 2; f++)
    {
      size_t end = filevec[f].buffered_lines;
      char *discards = discarded[f];
      lin *counts = equiv_count[1 - f];
      lin *equivs = filevec[f].equivs;
      size_t many = 5;
      size_t tem = end / 64;

      /* Multiply MANY by approximate square root of number of lines.
	 That is the threshold for provisionally discardable lines.  */
      while ((tem = tem >> 2) > 0)
	many *= 2;

      for (i = 0; i < end; i++)
	{
	  lin nmatch;
	  if (equivs[i] == 0)
	    continue;
	  nmatch = counts[equivs[i]];
	  if (nmatch == 0)
	    discards[i] = 1;
	  else if (nmatch > many)
	    discards[i] = 2;
	}
    }

  /* Don't really discard the provisional lines except when they occur
     in a run of discardables, with nonprovisionals at the beginning
     and end.  */

  for (f = 0; f < 2; f++)
    {
      lin end = filevec[f].buffered_lines;
      register char *discards = discarded[f];

      for (i = 0; i < end; i++)
	{
	  /* Cancel provisional discards not in middle of run of discards.  */
	  if (discards[i] == 2)
	    discards[i] = 0;
	  else if (discards[i] != 0)
	    {
	      /* We have found a nonprovisional discard.  */
	      register lin j;
	      lin length;
	      lin provisional = 0;

	      /* Find end of this run of discardable lines.
		 Count how many are provisionally discardable.  */
	      for (j = i; j < end; j++)
		{
		  if (discards[j] == 0)
		    break;
		  if (discards[j] == 2)
		    ++provisional;
		}

	      /* Cancel provisional discards at end, and shrink the run.  */
	      while (j > i && discards[j - 1] == 2)
		discards[--j] = 0, --provisional;

	      /* Now we have the length of a run of discardable lines
		 whose first and last are not provisional.  */
	      length = j - i;

	      /* If 1/4 of the lines in the run are provisional,
		 cancel discarding of all provisional lines in the run.  */
	      if (provisional * 4 > length)
		{
		  while (j > i)
		    if (discards[--j] == 2)
		      discards[j] = 0;
		}
	      else
		{
		  register lin consec;
		  lin minimum = 1;
		  lin tem = length >> 2;

		  /* MINIMUM is approximate square root of LENGTH/4.
		     A subrun of two or more provisionals can stand
		     when LENGTH is at least 16.
		     A subrun of 4 or more can stand when LENGTH >= 64.  */
		  while (0 < (tem >>= 2))
		    minimum <<= 1;
		  minimum++;

		  /* Cancel any subrun of MINIMUM or more provisionals
		     within the larger run.  */
		  for (j = 0, consec = 0; j < length; j++)
		    if (discards[i + j] != 2)
		      consec = 0;
		    else if (minimum == ++consec)
		      /* Back up to start of subrun, to cancel it all.  */
		      j -= consec;
		    else if (minimum < consec)
		      discards[i + j] = 0;

		  /* Scan from beginning of run
		     until we find 3 or more nonprovisionals in a row
		     or until the first nonprovisional at least 8 lines in.
		     Until that point, cancel any provisionals.  */
		  for (j = 0, consec = 0; j < length; j++)
		    {
		      if (j >= 8 && discards[i + j] == 1)
			break;
		      if (discards[i + j] == 2)
			consec = 0, discards[i + j] = 0;
		      else if (discards[i + j] == 0)
			consec = 0;
		      else
			consec++;
		      if (consec == 3)
			break;
		    }

		  /* I advances to the last line of the run.  */
		  i += length - 1;

		  /* Same thing, from end.  */
		  for (j = 0, consec = 0; j < length; j++)
		    {
		      if (j >= 8 && discards[i - j] == 1)
			break;
		      if (discards[i - j] == 2)
			consec = 0, discards[i - j] = 0;
		      else if (discards[i - j] == 0)
			consec = 0;
		      else
			consec++;
		      if (consec == 3)
			break;
		    }
		}
	    }
	}
    }

  /* Actually discard the lines. */
  for (f = 0; f < 2; f++)
    {
      char *discards = discarded[f];
      lin end = filevec[f].buffered_lines;
      lin j = 0;
      for (i = 0; i < end; ++i)
	if (minimal || discards[i] == 0)
	  {
	    filevec[f].undiscarded[j] = filevec[f].equivs[i];
	    filevec[f].realindexes[j++] = i;
	  }
	else
	  filevec[f].changed[i] = 1;
      filevec[f].nondiscarded_lines = j;
    }

  free (discarded[0]);
  free (equiv_count[0]);
}



/* Adjust inserts/deletes of identical lines to join changes
   as much as possible.

   We do something when a run of changed lines include a
   line at one end and have an excluded, identical line at the other.
   We are free to choose which identical line is included.
   `compareseq' usually chooses the one at the beginning,
   but usually it is cleaner to consider the following identical line
   to be the "change".  */

static void shift_boundaries (struct file_data filevec[])
{
  int f;

  for (f = 0; f < 2; f++)
    {
      bool *changed = filevec[f].changed;
      bool const *other_changed = filevec[1 - f].changed;
      lin const *equivs = filevec[f].equivs;
      lin i = 0;
      lin j = 0;
      lin i_end = filevec[f].buffered_lines;

      while (1)
	{
	  lin runlength, start, corresponding;

	  /* Scan forwards to find beginning of another run of changes.
	     Also keep track of the corresponding point in the other file.  */

	  while (i < i_end && !changed[i])
	    {
	      while (other_changed[j++])
		continue;
	      i++;
	    }

	  if (i == i_end)
	    break;

	  start = i;

	  /* Find the end of this run of changes.  */

	  while (changed[++i])
	    continue;
	  while (other_changed[j])
	    j++;

	  do
	    {
	      /* Record the length of this run of changes, so that
		 we can later determine whether the run has grown.  */
	      runlength = i - start;

	      /* Move the changed region back, so long as the
		 previous unchanged line matches the last changed one.
		 This merges with previous changed regions.  */

	      while (start && equivs[start - 1] == equivs[i - 1])
		{
		  changed[--start] = 1;
		  changed[--i] = 0;
		  while (changed[start - 1])
		    start--;
		  while (other_changed[--j])
		    continue;
		}

	      /* Set CORRESPONDING to the end of the changed run, at the last
		 point where it corresponds to a changed run in the other file.
		 CORRESPONDING == I_END means no such point has been found.  */
	      corresponding = other_changed[j - 1] ? i : i_end;

	      /* Move the changed region forward, so long as the
		 first changed line matches the following unchanged one.
		 This merges with following changed regions.
		 Do this second, so that if there are no merges,
		 the changed region is moved forward as far as possible.  */

	      while (i != i_end && equivs[start] == equivs[i])
		{
		  changed[start++] = 0;
		  changed[i++] = 1;
		  while (changed[i])
		    i++;
		  while (other_changed[++j])
		    corresponding = i;
		}
	    }
	  while (runlength != i - start);

	  /* If possible, move the fully-merged run of changes
	     back to a corresponding run in the other file.  */

	  while (corresponding < i)
	    {
	      changed[--start] = 1;
	      changed[--i] = 0;
	      while (other_changed[--j])
		continue;
	    }
	}
    }
}

/* Cons an additional entry onto the front of an edit script OLD.
   LINE0 and LINE1 are the first affected lines in the two files (origin 0).
   DELETED is the number of lines deleted here from file 0.
   INSERTED is the number of lines inserted here in file 1.

   If DELETED is 0 then LINE0 is the number of the line before
   which the insertion was done; vice versa for INSERTED and LINE1.  */

static struct change * add_change (lin line0, lin line1, lin deleted, lin inserted,
	    struct change *old)
{
  struct change *new = xmalloc (sizeof *new);

  new->line0 = line0;
  new->line1 = line1;
  new->inserted = inserted;
  new->deleted = deleted;
  new->link = old;
  return new;
}



/* Scan the tables of which lines are inserted and deleted,
   producing an edit script in forward order.  */

static struct change * build_script (struct file_data const filevec[])
{
  struct change *script = 0;
  bool *changed0 = filevec[0].changed;
  bool *changed1 = filevec[1].changed;
  lin i0 = filevec[0].buffered_lines, i1 = filevec[1].buffered_lines;

  /* Note that changedN[-1] does exist, and is 0.  */

  while (i0 >= 0 || i1 >= 0)
    {
      if (changed0[i0 - 1] | changed1[i1 - 1])
	{
	  lin line0 = i0, line1 = i1;

	  /* Find # lines changed here in each file.  */
	  while (changed0[i0 - 1]) --i0;
	  while (changed1[i1 - 1]) --i1;

	  /* Record this change.  */
	  script = add_change (i0, i1, line0 - i0, line1 - i1, script);
	}

      /* We have reached lines in the two files that match each other.  */
      i0--, i1--;
    }

  return script;
}

struct language_type * get_current_language_with_filename(char * filename)
{

	char * afterdot;
	char file_name [32];
	int  it_filename = strlen(filename);
	int it_language = 0;
	char * p;
	/*解决分析lost+found目录中一些奇怪的长文件出现越界的问题*/
	int skip_too_long = 28;

	/* get file extension name */
	afterdot = filename+strlen(filename);

	while (it_filename)
	{
		if ( *afterdot == '/' )
		{
			afterdot ++;
			break;
		}
		if (skip_too_long == 0)
		{
			it_filename = 0;
			break;
		}
		afterdot --;
		it_filename --;
		skip_too_long --;
	}

	if (it_filename == 0)
		return NULL;

 	/* use ";_ext_name_;" pattern */
	strcpy(file_name,";");
	strcat(file_name,afterdot);
	strcat(file_name,";");

	/*lower case */
	for (p=file_name;p<file_name+strlen(file_name);p++)
		*p = TOLOWER(*p);

	/*find language type based on file extension name */
	while ( languages[it_language].id != -1)
	{
		if (find_string(languages[it_language].file_names,file_name) != -1)
		{
			return &(languages[it_language]);
		}
		it_language ++;
	}

	return NULL;
}



struct language_type * get_current_language(char * filename)
{

	char * afterdot;
	char file_ext [20];
	int  it_filename = strlen(filename);
	int it_language = 0;
	char * p;
	/*解决分析lost+found目录中一些奇怪的长文件出现越界的问题*/
	int skip_too_long = 15;

	/* get file extension name */
	afterdot = filename+strlen(filename);

	while (it_filename)
	{
		if ( *afterdot == '.' )
		{
			afterdot ++;
			break;
		}
		if (skip_too_long == 0)
		{
			it_filename = 0;
			break;
		}
		afterdot --;
		it_filename --;
		skip_too_long --;
	}

	if (it_filename == 0)
		return NULL;

 	/* use ";_ext_name_;" pattern */
	strcpy(file_ext,";");
	strcat(file_ext,afterdot);
	strcat(file_ext,";");

	/*lower case */
	for (p=file_ext;p<file_ext+strlen(file_ext);p++)
		*p = TOLOWER(*p);

	/*find language type based on file extension name */
	while ( languages[it_language].id != -1)
	{
		if (find_string(languages[it_language].file_extensions,file_ext) != -1)
		{
			if 	(!languages[it_language].constructed)
				languages[it_language].constructed = 1;
			return &(languages[it_language]);
		}
		it_language ++;
	}

	return NULL;
}

int gbk2utf8(char *utfStr,const char *srcStr,int maxUtfStrlen)
{
    if(NULL==srcStr)
    {
       printf("Bad Parameter\n");
      return -1;
    }

    //首先先将gbk编码转换为unicode编码
    if(NULL==setlocale(LC_ALL,"zh_CN.GB18030"))//设置转换为unicode前的码,当前为gbk编码
    {
      printf("Bad Parameter\n");
      return -1;
   }

   int unicodeLen=mbstowcs(NULL,srcStr,0);//计算转换后的长度
    if(unicodeLen<=0)
    {
        printf("Can not Transfer!!!\n");
        return -1;
    }
   wchar_t *unicodeStr=(wchar_t *)calloc(sizeof(wchar_t),unicodeLen+1);
   mbstowcs(unicodeStr,srcStr,strlen(srcStr));//将gbk转换为unicode

    //将unicode编码转换为utf8编码
    if(NULL==setlocale(LC_ALL,"zh_CN.utf8"))//设置unicode转换后的码,当前为utf8
    {
        printf("Bad Parameter\n");
        return -1;
    }
    int utfLen=wcstombs(NULL,unicodeStr,0);//计算转换后的长度
   if(utfLen<=0)
    {
        printf("Can not Transfer!!!\n");
        return -1;
    }
    else if(utfLen>=maxUtfStrlen)//判断空间是否足够
    {
        printf("Dst Str memory not enough\n");
        return -1;
   }
    wcstombs(utfStr,unicodeStr,utfLen);
    utfStr[utfLen]=0;//添加结束符
    free(unicodeStr);
   return utfLen;
}

static void counting_diff_list(struct diffed_line_node *diffed_list, struct comparison *cmp)
{

	int total_added_lines=0,
		total_modified_lines=0,
		total_deleted_lines=0,

	    changed_comment_lines=0,
		changed_blank_lines=0,
		changed_nbnc_lines=0;

	/*控制各种注释的处理过程*/
	bool processing_block_comment=false,
		 changed_block_comment_process=false,
		 normal_block_comment_process=false;

	char * left_file_name;  /* 基线的文件名，如果新增为空 */
	char * right_file_name; /* 目标文件名，如果删除为空 */
	char * diff_state; /*diff的文件状态，文件是新增、删除、还是修改*/

	struct diffed_line_node * item = diffed_list;

	while (item)
	{
		switch (item->line_status)
		{
			case CHANGED:
				total_modified_lines +=1;
				break;
			case OLD:
				total_deleted_lines +=1;
				break;
			case NEW:
				total_added_lines +=1;
				break;
			default:
				break;
		}

		if (!processing_block_comment)
		{
			if ((item->line_status == CHANGED )||(item->line_status == NEW))
			{
				switch(item->line_comment)
				{
					case NormalLine:
						changed_nbnc_lines += 1;
						break;
					case LineComment:
						changed_comment_lines += 1;
						break;
					case NormalWithComment:
						changed_comment_lines += 1;
						changed_nbnc_lines += 1;
						break;
					case BlankLine:
						changed_blank_lines += 1;
						break;
					case BlockCommentOn:
						changed_comment_lines +=1;
						processing_block_comment = true;
						changed_block_comment_process = true;
						break;
					case NormalWithBlockOn:
						changed_comment_lines +=1;
						changed_nbnc_lines +=1;
						processing_block_comment = true;
						changed_block_comment_process = true;
						break;
					}
			} /* changed or new */
			else
			if (item->line_status == UNCHANGED )
			{
				switch(item->line_comment)
				{
					case BlockCommentOn:
						processing_block_comment = true;
						normal_block_comment_process = true;
						break;
					case NormalWithBlockOn:
						processing_block_comment = true;
						normal_block_comment_process = true;
						break;
				}
			}

		}/* ! process_block_comment */
		else /* process_block_comment */
		{
			if (changed_block_comment_process)
			{
				if ((item->line_status == CHANGED )||(item->line_status == NEW))
					{
						switch(item->line_comment)
						{
							case BlankLine:
								changed_blank_lines += 1;
								break;
							case BlockCommentOff:
								changed_comment_lines +=1;
								processing_block_comment = false;
								changed_block_comment_process = false;
								/*防止块注释符修改一半*/
								normal_block_comment_process = false;
								break;
							case BlockOffWithNormal:
								changed_comment_lines +=1;
								changed_nbnc_lines += 1;
								processing_block_comment = false;
								changed_block_comment_process = false;
								/*防止块注释符修改一半*/
								normal_block_comment_process = false;
								/*Add by caoyang for BUG fix */
								break;
								/*Add by caoyang end */
							default:
								changed_comment_lines +=1;
								break;
						}
					}
				    /* add by caoyang 2009 ,fix a non-end comment bug */
					else if (item->line_status == UNCHANGED )
							switch(item->line_comment)
							{
								case BlockCommentOff:
								case BlockOffWithNormal:
									processing_block_comment = false;
									normal_block_comment_process = false;
									changed_block_comment_process = false;
								break;
							}
					/* add end */
			}/* if changed_block_comment_process */
			else
			if (normal_block_comment_process)
			{
			    /*Add by caoyang for fix:如果变化的代码（新增修改）
			    都在一个大块的注释内部，进行，在后边统计的时候，都会被略过不处理*/
			   	if ((item->line_status == CHANGED )||(item->line_status == NEW))
			   	{
			   	   switch(item->line_comment)
	   			   {

		      			case BlankLine:
								changed_blank_lines += 1;
								break;
						case BlockCommentOff:
								changed_comment_lines +=1;
								processing_block_comment = false;
								normal_block_comment_process = false;
								/*防止块注释符修改一半*/
								changed_block_comment_process = false;
								break;
						case BlockOffWithNormal:
								changed_comment_lines += 1;
								changed_nbnc_lines += 1;
								processing_block_comment = false;
								normal_block_comment_process = false;
								/*防止块注释符修改一半*/
								changed_block_comment_process = false;
								break;
						default:
								changed_comment_lines +=1;
								break;
			   	   	}
			   	}
			   	else if (item->line_status == UNCHANGED )
					   	/*Add by caoyang for fix bug end */
						switch(item->line_comment)
						{
							case BlockCommentOff:
							case BlockOffWithNormal:
								processing_block_comment = false;
								normal_block_comment_process = false;
								/*防止块注释符修改一半*/
								changed_block_comment_process = false;
							break;
						}
			}

		}/* end of if process_block_comment */

		item = item->next;
	}


	current_language->total_added_lines += total_added_lines;
	current_language->total_deleted_lines += total_deleted_lines;
	current_language->total_modified_lines += total_modified_lines;

	current_language->changed_comment_lines += changed_comment_lines;
	current_language->changed_blank_lines += changed_blank_lines;
	current_language->changed_nbnc_lines += changed_nbnc_lines;

	left_file_name = cmp->file[0].name;
	right_file_name = cmp->file[1].name;

	if (cmp->file[0].desc == -1)
	{
	 left_file_name = "";
	 diff_state = "NEW";
	}
	else if (cmp->file[1].desc == -1)
	 {
		 right_file_name = "";
		 diff_state = "DEL";
	 }
	else
	 diff_state = "MOD";

	 /* if set print_files_info, print the mid result */
	if (print_files_info)
	{
		/*--------------- 进行详细清单(包括文件信息)的结果输出----------------------*/
		/*重新打开全路径*/


		if (counting_only)
		{
			printf("%s",current_language->language_name);
			printf("\t%d",total_added_lines);
			printf("\t%d",changed_blank_lines);
			printf("\t%d",changed_comment_lines);
			printf("\t%d",changed_nbnc_lines);
			printf("\t%s\n",right_file_name);
		}
    	/* -- Debug Output one file diff and count result */
    	else
    	{
    		printf("%s",current_language->language_name);
			printf("\t%d",total_added_lines);
			printf("\t%d",total_modified_lines);
			printf("\t%d",total_deleted_lines);
			printf("\t%d" ,total_added_lines + total_modified_lines);
			printf("\t%d",changed_blank_lines);
			printf("\t%d",changed_comment_lines);
			printf("\t%d",changed_nbnc_lines);
			printf("\t%s",diff_state);
			printf("\t%s",left_file_name);
			printf("\t%s\n",right_file_name);
    	}
	}

	if(git_post_commit_counting )
	{
		insert_code_count(svn_info.repos_id,svn_info.revision,svn_info.svn_url_path,
			svn_info.user_id,svn_info.date,current_language->language_name,diff_state,total_added_lines,total_modified_lines,total_deleted_lines,changed_blank_lines,changed_comment_lines,changed_nbnc_lines);
	}

}

bool insert_code_count(char *repos_id,char *revision,char *svn_url_path,char *user_id,char *date,char *language_name,char *diff_state,int total_added_lines,int total_modified_lines,int total_deleted_lines,int changed_blank_lines,int changed_comment_lines,int changed_nbnc_lines)
{
	if(NULL == repos_id || NULL == revision || NULL == diff_state || NULL == svn_url_path || NULL == user_id || NULL == date || NULL == language_name)
	{
		SVN_COUNT_DEBUG(" empty  %s %s %s %s %s\n",repos_id,revision,diff_state,svn_url_path,user_id);
		return false;
	}

	int len = strlen(svn_url_path) +strlen(repos_id) +strlen(revision) +strlen(user_id) + strlen(svn_info.branch) + strlen(svn_info.server_id) + strlen(svn_info.server_ip) + 512;
	char *execsql = (char *)malloc(len);
	char *execsql_utf8 = (char *)malloc(len*2 +1);
	SVN_COUNT_DEBUG(" len  : %d \n",len);

	if(NULL == svn_info.commit_level)
		svn_info.commit_level = CODE_COUNT_NORMAL;

	if(changed_nbnc_lines >= 2000 && strcmp(svn_info.commit_level,CODE_COUNT_REPLACE) != 0)
	{
		svn_info.commit_level = CODE_COUNT_WARNING;
	}

	sprintf(execsql,"insert into git_code_count(server_id,repos_id,revision,path,user_id,date,diff_state,added_lines,modified_lines,deleted_lines,changed_blank_lines,changed_comment_lines,changed_nbnc_lines,language_type,commit_level,branch,grandpa_revision) \
	values(%s,%s,\"%s\",\"%s\",%s,\"%s\",\"%s\",%d,%d,%d,%d,%d,%d,\"%s\",\"%s\",\"%s\",\"%s\")",svn_info.server_id,repos_id,revision,svn_url_path,
	user_id,date,diff_state,total_added_lines,total_modified_lines,total_deleted_lines,changed_blank_lines,
	changed_comment_lines,changed_nbnc_lines,language_name,svn_info.commit_level,svn_info.branch,svn_info.grandpa_revision);
	SVN_COUNT_DEBUG("execsql : %s \n",execsql);

	/*if(gbk2utf8(execsql_utf8,execsql,len*2 +1) == -1)
	{
		free(execsql);
		free(execsql_utf8);
		return false;
	}*/
	//SVN_COUNT_DEBUG("execsql_utf8 : %s \n",execsql_utf8);
	if(git_count_update)
	{
		svn_count_mysql_query(&mysql_conn,execsql);
		sprintf(execsql,"update git_code_count set diff_state=\"%s\",added_lines=%d,modified_lines=%d,deleted_lines=%d,changed_blank_lines=%d,changed_comment_lines=%d,changed_nbnc_lines=%d \
		where server_id=%s and repos_id=%s and revision=%s and path=\"%s\" ",diff_state,total_added_lines,total_modified_lines,total_deleted_lines,changed_blank_lines,
		changed_comment_lines,changed_nbnc_lines,svn_info.server_id,repos_id,revision,svn_url_path);
		memset(execsql_utf8,0,len*2 +1);
		/*if(gbk2utf8(execsql_utf8,execsql,len*2 +1) == -1)
		{
			free(execsql);
			free(execsql_utf8);
			return false;
		}*/
		if(svn_count_mysql_query(&mysql_conn,execsql))
		{
			//可能是统计出错的代码，记录，后续分析
			sprintf(execsql,"insert into git_code_count_error(server_id,repos_id,revision,update_time,grandpa_revision) values(%s,%s,\"%s\",now(),\"%s\");",svn_info.server_id,repos_id,revision,svn_info.grandpa_revision);
			SVN_COUNT_DEBUG("### error count ### execsql : %s \n",execsql);
			svn_count_mysql_query(&mysql_conn,execsql);
			//e/xit(-1);
		}
	}
	else
	{
		svn_count_mysql_query(&mysql_conn,execsql);

	}
	free(execsql);
	free(execsql_utf8);
	return true;
}


/* Scan the differences of two files.  */
int diff_2_files_memery (struct comparison *cmp,char *buffer_new,char *buffer_old)
{

  lin diags;
  int f;
  struct change *e, *p;
  struct change *script;
  int changes;
  char * file_basename;

  printf("here");

  trace3("--->Enter diff_2_files(): file1 is %s, file2 is %s \n",cmp->file[0].name,cmp->file[1].name);
  if (print_lines_info )
  	   printf("differ %s, %s \n",cmp->file[0].name,cmp->file[1].name);

  file_basename = base_name(cmp->file[1].name);

  	trace2("%s Pattern to the language define ...Ok!\n",file_basename);

  	/* 到这个地方为止,cmp中的两个文件除了文件名和描述之外
  	   还没有其他的信息，尤其是BUFFER为空*/
  	if (!read_files_memery (cmp->file,buffer_new,buffer_old))
  	{
  		/* 真正的处理过程只针对纯文本文件(代码)进行*/


  	   /*对于文本文件，则进行正常处理
  	      这个时候，cmp中的file的Buffer信息就已经存在了*/
  	    /* Allocate vectors for the results of comparison:
			 a flag for each line of each file, saying whether that line
			 is an insertion or deletion.
			 Allocate an extra element, always 0, at each end of each vector.  */
  	 printf("job done diff_2_files_memery 1\n");
  	    size_t s = cmp->file[0].buffered_lines + cmp->file[1].buffered_lines + 4;
		 printf("s  : %ld\n",s);
  	    bool *flag_space = zalloc (s * sizeof *flag_space);
  	    cmp->file[0].changed = flag_space + 1;
  	    cmp->file[1].changed = flag_space + cmp->file[0].buffered_lines + 3;

  	    /* Some lines are obviously insertions or deletions
			 because they don't match anything.  Detect them now, and
			 avoid even thinking about them in the main comparison algorithm.  */

  	    discard_confusing_lines (cmp->file);

  	    /* Now do the main comparison algorithm, considering just the
			 undiscarded lines.  */

  	    xvec = cmp->file[0].undiscarded;
  	    yvec = cmp->file[1].undiscarded;
  	    diags = (cmp->file[0].nondiscarded_lines
			       + cmp->file[1].nondiscarded_lines + 3);
  	    fdiag = xmalloc (diags * (2 * sizeof *fdiag));
  	    bdiag = fdiag + diags;
  	    fdiag += cmp->file[1].nondiscarded_lines + 1;
  	    bdiag += cmp->file[1].nondiscarded_lines + 1;

  	    /* Set TOO_EXPENSIVE to be approximate square root of input size,
			 bounded below by 256.  */
  	    too_expensive = 1;
  	    for (;  diags != 0;  diags >>= 2)
			  too_expensive <<= 1;
  	    too_expensive = MAX (256, too_expensive);

  	    files[0] = cmp->file[0];
  	    files[1] = cmp->file[1];

  	    compareseq (0, cmp->file[0].nondiscarded_lines,
				  0, cmp->file[1].nondiscarded_lines, minimal);

  	    free (fdiag - (cmp->file[1].nondiscarded_lines + 1));

  	    /* Modify the results slightly to make them prettier
			 in cases where that can validly be done.  */

  	    shift_boundaries (cmp->file);

  	    /* Get the results of comparison in the form of a chain
			 of `struct change's -- an edit script.  */

  	    /*最终得到的差异结果(Only Diff)*/
  	 	 script = build_script (cmp->file);
		printf("job done diff_2_files_memery 2\n");

		/* Set CHANGES if we had any diffs. */

  	   changes = (script != 0);

		if (changes)
		{
		  /* print file information ... debug的时候通过参数显示*/
		  #ifdef DIFFCOUNT_DEBUG
		  if (print_files_info )
		  {
				if (counting_only)
		  		{
				   if (cmp->parent != 0)
							printf("Process counting file: %s \n",cmp->file[1].name);
				}
			  	else
				{
					 if (cmp->parent != 0)
						printf("Process diffcount: %s and %s \n",cmp->file[0].name,cmp->file[1].name);
				}
		  	}//end if print_file_info
		  	#endif

		  process_diff_script (script);
			printf("job done diff_2_files_memery 3\n");

		  /* Counting the diffed list */
		  counting_diff_list(line_node_head,cmp);

		 }


  	    free (cmp->file[0].undiscarded);
  	    free (flag_space);
  	    for (f = 0; f < 2; f++)
		{
			  free (cmp->file[f].equivs);
			  free (cmp->file[f].linbuf + cmp->file[f].linbuf_base);
		}

  	  	for (e = script; e; e = p)
		{
			  p = e->link;
			  free (e);
		}
		printf("job done diff_2_files_memery 4\n");

  	}

  //统一释放buffer
  if (cmp->file[0].buffer != cmp->file[1].buffer)
 		 free (cmp->file[0].buffer);
   free (cmp->file[1].buffer);

  trace3("<--Quit diff_2_files function..... \n");
  printf("job done diff_2_files_memery\n");

  return changes;

}






/* Scan the differences of two files.  */
int diff_2_files (struct comparison *cmp)
{

  lin diags;
  int f;
  struct change *e, *p;
  struct change *script;
  int changes;
  char * file_basename;
  trace3("--->Enter diff_2_files(): file1 is %s, file2 is %s \n",cmp->file[0].name,cmp->file[1].name);
  if (print_lines_info )
  	   printf("differ %s, %s \n",cmp->file[0].name,cmp->file[1].name);

  file_basename = base_name(cmp->file[1].name);
  if (current_language = get_current_language(file_basename))
  {
  	trace2("%s Pattern to the language define ...Ok!\n",file_basename);

  	/* 到这个地方为止,cmp中的两个文件除了文件名和描述之外
  	   还没有其他的信息，尤其是BUFFER为空*/
  	if (read_files (cmp->file))
  	{

		/*如果是二进制文件，一版情况下直接跳过，如果是DEBUG程序，用参数可以打印出来看看*/
  		#ifdef DIFFCOUNT_DEBUG

		/*对于二进制的文件，就简单的处理过去即可*/
  	    /* Files with different lengths must be different.  */
  	    if (cmp->file[0].stat.st_size != cmp->file[1].stat.st_size
			  && (cmp->file[0].desc < 0 || S_ISREG (cmp->file[0].stat.st_mode))
			  && (cmp->file[1].desc < 0 || S_ISREG (cmp->file[1].stat.st_mode)))
  	    	{
					changes = 1;
					trace2("##They are binary and They are NOT SAME !,change =1 \n");
  	    	}
  	    else
  	    	{
					changes = 0;
					trace2("##They are binary and They are SAME! ,change =0 \n");
  	    	}


		if (print_files_info )
		{
			  	if (counting_only)
						printf("Escape Binary file: %s \n",cmp->file[1].name);
			  	else
						printf("Escape Binary files: %s and %s \n",cmp->file[0].name,cmp->file[1].name);
		}//end if print_file_info

		#endif

	}
  	else
  		/* 真正的处理过程只针对纯文本文件(代码)进行*/
  	  {
  	   /*对于文本文件，则进行正常处理
  	      这个时候，cmp中的file的Buffer信息就已经存在了*/
  	    /* Allocate vectors for the results of comparison:
			 a flag for each line of each file, saying whether that line
			 is an insertion or deletion.
			 Allocate an extra element, always 0, at each end of each vector.  */

  	    size_t s = cmp->file[0].buffered_lines + cmp->file[1].buffered_lines + 4;
  	    bool *flag_space = zalloc (s * sizeof *flag_space);
  	    cmp->file[0].changed = flag_space + 1;
  	    cmp->file[1].changed = flag_space + cmp->file[0].buffered_lines + 3;

  	    /* Some lines are obviously insertions or deletions
			 because they don't match anything.  Detect them now, and
			 avoid even thinking about them in the main comparison algorithm.  */

  	    discard_confusing_lines (cmp->file);

  	    /* Now do the main comparison algorithm, considering just the
			 undiscarded lines.  */

  	    xvec = cmp->file[0].undiscarded;
  	    yvec = cmp->file[1].undiscarded;
  	    diags = (cmp->file[0].nondiscarded_lines
			       + cmp->file[1].nondiscarded_lines + 3);
  	    fdiag = xmalloc (diags * (2 * sizeof *fdiag));
  	    bdiag = fdiag + diags;
  	    fdiag += cmp->file[1].nondiscarded_lines + 1;
  	    bdiag += cmp->file[1].nondiscarded_lines + 1;

  	    /* Set TOO_EXPENSIVE to be approximate square root of input size,
			 bounded below by 256.  */
  	    too_expensive = 1;
  	    for (;  diags != 0;  diags >>= 2)
			  too_expensive <<= 1;
  	    too_expensive = MAX (256, too_expensive);

  	    files[0] = cmp->file[0];
  	    files[1] = cmp->file[1];

  	    compareseq (0, cmp->file[0].nondiscarded_lines,
				  0, cmp->file[1].nondiscarded_lines, minimal);

  	    free (fdiag - (cmp->file[1].nondiscarded_lines + 1));

  	    /* Modify the results slightly to make them prettier
			 in cases where that can validly be done.  */

  	    shift_boundaries (cmp->file);

  	    /* Get the results of comparison in the form of a chain
			 of `struct change's -- an edit script.  */

  	    /*最终得到的差异结果(Only Diff)*/
  	 	 script = build_script (cmp->file);

		/* Set CHANGES if we had any diffs. */

  	   changes = (script != 0);

		if (changes)
		{
		  /* print file information ... debug的时候通过参数显示*/
		  #ifdef DIFFCOUNT_DEBUG
		  if (print_files_info )
		  {
				if (counting_only)
		  		{
				   if (cmp->parent != 0)
							printf("Process counting file: %s \n",cmp->file[1].name);
				}
			  	else
				{
					 if (cmp->parent != 0)
						printf("Process diffcount: %s and %s \n",cmp->file[0].name,cmp->file[1].name);
				}
		  	}//end if print_file_info
		  	#endif

		  process_diff_script (script);

		  /* Counting the diffed list */
		  counting_diff_list(line_node_head,cmp);

		 }


  	    free (cmp->file[0].undiscarded);
  	    free (flag_space);
  	    for (f = 0; f < 2; f++)
		{
			  free (cmp->file[f].equivs);
			  free (cmp->file[f].linbuf + cmp->file[f].linbuf_base);
		}

  	  	for (e = script; e; e = p)
		{
			  p = e->link;
			  free (e);
		}

  	}
  }
  else
  {
  		//无法匹配到具体的语言类型的，直接给予刨除。如果不是debug状态，直接跳过不处理
  		#ifdef DIFFCOUNT_DEBUG

  		if (print_files_info )
		{
			  	if (counting_only)
						printf("Escape File Not Match Language: %s \n",cmp->file[1].name);
			  	else
						printf("Escape Files Not Match Language: %s and %s \n",cmp->file[0].name,cmp->file[1].name);
		}//end if print_file_info

		#endif
  	}


  //统一释放buffer
  if (cmp->file[0].buffer != cmp->file[1].buffer)
 		 free (cmp->file[0].buffer);
   free (cmp->file[1].buffer);

  trace3("<--Quit diff_2_files function..... \n");

  return changes;

}

