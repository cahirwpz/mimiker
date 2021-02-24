#!/bin/sh

# Set CPU time soft limit
limit=$1
shift
ulimit -St $limit

exec "$@"
