# 0.95

* not single header anymore (this was just too much of a hassle).
* custom allocator not quite ready yet. Checked in, but still using old allocator (easy to toggle - line 15 of spp_config.h)


# 0.90

* stable release (single header)
* known issues:
   -  memory usage can be excessive in Windows

      sparsepp has a very simple default allocator based on the system malloc/realloc/free implementation,
      and the default Windows realloc() appears to fragment the memory, causing significantly higher 
      memory usage than on linux. To solve this issue, I am working on a new allocator which will 
      remedy the problem.
