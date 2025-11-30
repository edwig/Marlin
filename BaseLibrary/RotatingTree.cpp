/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: RotatingTree.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/***********************************************************************\
*									                                                      *
*	      ROTATING_TREE.C	  		                                          *
*									                                                      *
*	      A balanced tree algorithme for binary tree's			              *
*	      The binary search tree is post-balanced on the tree insertions	*
*	      at the extra cost of one byte per treenode. Thus 2^256 nodes	  *
*	      (10^77) can be accounted. 					                            *
*                                                                       *
*       Insertions:                                                     *
*	      The worst case performance of this algorithme is less than	    *
*	      1/2 rotation per insertion. The extra time cost is made alsoo	  *
*	      on insertions, no extra routine time for balancing the tree	    *
*	      is requiered.							                                      *
*									                                                      *
*       Searches:                                                       *
*       Nominal 2logN lookups if N is the number of nodes in the tree   *
*                                                                       *
*       Deletions:                                                      *
*       Deletions can trigger rotations as do insertions, but on        *
*       delete triggers less than 1/2 rotation per deletion.            *
*       but uses (1/2 N logN ) search time!                             *
*                                                                       *
*       Outperforms STL std::map by a factor of 50%                     *
*                                                                       *
*       Edwig Huisman 1996                                              *
*                                                                       *
\***********************************************************************/

#include "pch.h"
#include "RotatingTree.h"
#include <functional>

/* For brevity a max */
#define max(a,b)	(((a) > (b)) ? (a) : (b))

RotatingTree::RotatingTree(RTCompareFunction p_compare)
	           :m_compare(p_compare)
{
}

RotatingTree::~RotatingTree()
{
	RemoveAll();
}

// Insert a symbol in the tree
void 
RotatingTree::Insert(RTKEY p_symbol,PAYLOAD p_payload)
{
	m_root = tree_insert(m_root,p_symbol,p_payload);
}

// Delete a symbol from the tree
void
RotatingTree::Delete(RTKEY p_symbol)
{
	m_root = tree_delete(m_root,p_symbol);
}

// Find a symbol in the tree
PAYLOAD
RotatingTree::Find(RTKEY p_symbol) const
{
	return findtree(m_root,p_symbol);
}

// Total size of the tree index
unsigned
RotatingTree::Size()
{
	return m_size;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void
RotatingTree::RemoveAll()
{
	// Recursive removal of all nodes
	std::function<void(RTREE*)> removeNode = [&](RTREE* node)
	{
		if(node)
		{
			removeNode(node->m_left);
			removeNode(node->m_right);
			delete [] node;
		}
	};
	removeNode(m_root);
	m_root = nullptr;
	m_size = 0;
}

// Insertion of one more symbol in the tree
RTREE*
RotatingTree::tree_insert(RTREE *p_branch,RTKEY p_symbol,PAYLOAD p_payload)
{
	long comp;
	
	if(!p_branch)
	{
		/* Insert a new leave	*/
		p_branch 		        = alloc_new RTREE();
		p_branch->m_symbol  = p_symbol;
		p_branch->m_payload = p_payload;
		p_branch->m_depth   = 1;
		p_branch->m_left   	=
		p_branch->m_right	  = NULL;

		// One more node in the tree
		++m_size;

		return p_branch;
	}
	if((comp = (*m_compare)(p_branch->m_symbol,p_symbol)) == 0)
	{
    // Symbol already in tree
		return p_branch;
	}
	if(comp > 0)
	{
		/* Out on a left branch	*/
		p_branch->m_left = tree_insert(p_branch->m_left,p_symbol,p_payload);
	}
	else
	{
		/* Out on a right branch */
		p_branch->m_right = tree_insert(p_branch->m_right,p_symbol,p_payload);
	}
  // Rotate once if necessary
	return rotate(p_branch);
}

// Rebalance the tree by rotating if necessary
RTREE*
RotatingTree::rotate(RTREE* p_branch)
{
  int	left = 0;
  int	right = 0;

  if(p_branch->m_left)
  {
    left = p_branch->m_left->m_depth;
  }
  if(p_branch->m_right)
  {
    right = p_branch->m_right->m_depth;
  }
  if(left - right > 1)
  {
    p_branch = rotate_left(p_branch);
  }
  else if(right - left > 1)
  {
    p_branch = rotate_right(p_branch);
  }
  set_depth(p_branch);
  return p_branch;
}

// Rotate on the left side
RTREE*
RotatingTree::rotate_left(RTREE* p_branch)
{
  RTREE* l = p_branch->m_left;
  p_branch->m_left = l->m_right;
  l->m_right = p_branch;
  p_branch = l;
  set_depth(p_branch->m_right);

  return p_branch;
}

// Rotate on the right side
RTREE*
RotatingTree::rotate_right(RTREE* p_branch)
{
  RTREE* r = p_branch->m_right;
  p_branch->m_right = r->m_left;
  r->m_left = p_branch;
  p_branch = r;
  set_depth(p_branch->m_left);

  return p_branch;
}

// Delete one (1) branch of the tree, specfied by symbol. Re-balance
// the tree afterward to keep it in balance.

RTREE*
RotatingTree::tree_delete(RTREE* p_branch,RTKEY p_symbol)
{
	long	comp;
	int	did_delete = 0;

	if(!p_branch)
	{
		return NULL;
	}
	if((comp = (*m_compare)(p_branch->m_symbol,p_symbol)) == 0)
	{
		/* Remove this branch					*/
		RTREE* original = p_branch;
		RTREE* chain;
		int left,right;

		if(!p_branch->m_left && !p_branch->m_right)
		{
			/* This was the root branch			*/
			p_branch = nullptr;
		}
		else if(!p_branch->m_left)
		{
			/* Simple case. Left branch is missing		*/
			p_branch = p_branch->m_right;
		}
		else if(!p_branch->m_right)
		{
			/* Simple case. Right branch is missing		*/
			p_branch = p_branch->m_left;
		}
		else
		{
			/* Both branches are there			*/
			left  = p_branch->m_left->m_depth;
			right = p_branch->m_right->m_depth;
			if(left >= right)
			{
				chain = p_branch->m_left;
				while(chain->m_right)
				{
					chain = chain->m_right;
				}
				chain->m_right = p_branch->m_right;
				p_branch = original->m_left;
			}
			else
			{
				chain = p_branch->m_right;
				while(chain->m_left)
				{
					chain = chain->m_left;
				}
				chain->m_left = p_branch->m_left;
				p_branch = original->m_right;
			}
		}
		delete [] original;
		did_delete = 1;

		// One less node in the tree
		--m_size;
	}
	else if(comp > 0)
	{
		/* Out on a left branch					*/
		if(p_branch->m_left)
		{
			p_branch->m_left = tree_delete(p_branch->m_left,p_symbol);
		}
		else
		{
			/* ELSE symbol is not in the tree		*/
			return p_branch;
		}
	}
	else
	{
		/* Out on a right branch				*/
		if(p_branch->m_right)
		{
			p_branch->m_right = tree_delete(p_branch->m_right,p_symbol);
		}
		else
		{
			/* ELSE symbol is not in the tree		*/
			return p_branch;
		}
	}
	if(did_delete && p_branch)
	{
		p_branch = rebalance(p_branch);
	}
	return p_branch;
}

// Rebalance a (part of a) rotating tree after a delete of a branch
// Must be done to keep the tree in shape.

RTREE*
RotatingTree::rebalance(RTREE* p_branch)
{
  if(!p_branch)
  {
    return NULL;
  }
  if(p_branch->m_left)
  {
    p_branch->m_left = rebalance(p_branch->m_left);
  }
  if(p_branch->m_right)
  {
    p_branch->m_right = rebalance(p_branch->m_right);
  }
  p_branch = rotate(p_branch);
  return p_branch;
}

void
RotatingTree::set_depth(RTREE* p_branch)
{
  int	left = 0;
  int	right = 0;

	if(!p_branch)
	{
		return;
	}
  if(p_branch->m_left)
  {
    left = p_branch->m_left->m_depth;
  }
  if(p_branch->m_right)
  {
    right = p_branch->m_right->m_depth;
  }
  p_branch->m_depth = 1 + max(left,right);
}

// Recursive find a node in the tree

PAYLOAD
RotatingTree::findtree(RTREE* p_branch,RTKEY p_symbol) const
{
  long comp;

  if(!p_branch)
  {
    return nullptr;
  }
  if((comp = (*m_compare)(p_branch->m_symbol,p_symbol)) == 0)
  {
    return p_branch->m_payload;
  }
  if(comp > 0)
  {
    /* Out on a left branch	*/
    return findtree(p_branch->m_left,p_symbol);
  }
  /* Out on a right branch */
  return findtree(p_branch->m_right,p_symbol);
}
