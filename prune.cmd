echo Pruning the tree
echo .
git fsck
git gc
git prune --expire now
git fsck
echo .
echo Ready pruning the tree
