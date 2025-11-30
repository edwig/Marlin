/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: RotatingTree.h
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
#pragma once

typedef void* RTKEY;         // The key to use
typedef void* PAYLOAD;       // What we store/retrieve

typedef	struct	_rtree		   // Rotating tree for locks
{
  RTKEY	   m_symbol;		     // Symbol to store
  PAYLOAD  m_payload;        // What we store and retrieve
  unsigned m_depth;		       // Depth of the tree
  struct	 _rtree* m_left;   // Binary left  branch
  struct	 _rtree* m_right;  // Binary right branch
}
RTREE;

// Rotating tree comparison function
// Should return [negative, zero, positive] integer
typedef int (*RTCompareFunction)(const RTKEY p_left,const RTKEY p_right);

class RotatingTree
{
public:
  RotatingTree(RTCompareFunction p_compare);
 ~RotatingTree();

  // Insert a symbol in the tree
  void     Insert(RTKEY p_symbol,PAYLOAD p_payload);
  // Delete a symbol from the tree
  void     Delete(RTKEY p_symbol);
  // Find a symbol in the tree
  PAYLOAD  Find(RTKEY p_symbol) const;
  // Remove the entire tree
  void     RemoveAll();
  // Total size of the tree index
  unsigned Size();

private:
  // Internal tree insert function
  RTREE* tree_insert (RTREE* p_branch,RTKEY p_symbol,PAYLOAD p_payload);
  RTREE* rotate      (RTREE* p_branch);
  RTREE* rotate_left (RTREE* p_branch);
  RTREE* rotate_right(RTREE* p_branch);
  // Internal deletion
  RTREE* tree_delete(RTREE* p_branch,RTKEY p_symbol);
  RTREE* rebalance  (RTREE* p_branch);
  void   set_depth  (RTREE* p_branch);
  // Internal Finding a node
  PAYLOAD findtree(RTREE* p_branch,RTKEY p_symbol) const;

  // DATA

  // Here lives our rotating tree
  mutable RTREE* m_root { nullptr };
  // Total count of nodes in the tree
  unsigned m_size  { 0 };
  // Here is our comparison function
  RTCompareFunction m_compare { nullptr };
};
