#include <rman.h>

// TODO all this logic associated with blocks is not really tested

static rman_block_t *find_block(rman_t *rm, rman_addr start, rman_addr end,
                                rman_addr count) {
  for (rman_block_t *block = rm->blocks.lh_first; block != NULL;
       block = block->blocks.le_next) {
    if (block->is_allocated)
      continue;

    // calculate common part and check if is big enough
    rman_addr s = max(start, block->start);
    rman_addr e = min(end, block->end);

    rman_addr len = s - e + 1;

    if (len >= count) {
      return block;
    }
  }

  return NULL;
}

// divide block into two
// `where` means start of right block
// function returns pointer to new (right) block
static rman_block_t *cut_block(rman_block_t *block, rman_addr where) {
  assert(where > block->start);
  assert(where < block->end);

  rman_block_t *left_block = block;
  rman_block_t *right_block = kmalloc(M_DEV, sizeof(rman_block_t), M_ZERO);

  left_block->blocks.le_next = right_block;
  right_block->blocks.le_prev = &left_block;

  right_block->end = left_block->end;
  right_block->start = where;
  left_block->end = where - 1;

  return right_block;
}

// maybe split block into two or three in order to recover space before and
// after allocation
static rman_block_t *split_block(rman_block_t *block, rman_addr start,
                                 rman_addr end, rman_addr count) {
  if (block->start < start) {
    block = cut_block(block, start);
  }

  if (block->end > block->start + count - 1) {
    block = cut_block(block, block->start + count);
  }

  return block;
}

void rman_allocate_resource(resource_t *res, rman_t *rm, rman_addr start,
                            rman_addr end, rman_addr count) {
  SCOPED_MTX_LOCK(&rm->mtx);

  rman_block_t *block = find_block(rm, start, end, count);
  assert(block != NULL); // TODO maybe this exception should be recoverable?

  block = split_block(block, start, end, count);
  block->is_allocated = true;
  res->r_start = block->start;
  res->r_end = block->end;

  // TODO alignment

  rm->start += count;
}

void rman_init(rman_t *rm) {
  mtx_init(&rm->mtx, MTX_DEF);
  LIST_INIT(&rm->blocks);

  // TODO so maybe we don't need to store start and end in rman_t?
  rman_block_t *whole_space = kmalloc(M_DEV, sizeof(rman_block_t), M_ZERO);

  whole_space->start = rm->start;
  whole_space->end = rm->end;
  whole_space->is_allocated = false;

  LIST_INSERT_HEAD(&rm->blocks, whole_space, blocks);
}
