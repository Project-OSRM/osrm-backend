# Readme

For instructions on how to compile and run OSRM, please consult the Wiki at

https://github.com/DennisOSRM/Project-OSRM/wiki

or use our free and daily updated online service at

http://map.project-osrm.org

## References in publications

When using the code in a (scientific) publication, please cite

```
@inproceedings{luxen-vetter-2011,
 author = {Luxen, Dennis and Vetter, Christian},
 title = {Real-time routing with OpenStreetMap data},
 booktitle = {Proceedings of the 19th ACM SIGSPATIAL International Conference on Advances in Geographic Information Systems},
 series = {GIS '11},
 year = {2011},
 isbn = {978-1-4503-1031-4},
 location = {Chicago, Illinois},
 pages = {513--516},
 numpages = {4},
 url = {http://doi.acm.org/10.1145/2093973.2094062},
 doi = {10.1145/2093973.2094062},
 acmid = {2094062},
 publisher = {ACM},
 address = {New York, NY, USA},
}
```

## Current build status

| build config |  branch | status |
|:-------------|:--------|:------------|
| Project OSRM | master  | [![Build Status](https://travis-ci.org/DennisOSRM/Project-OSRM.png?branch=master)](https://travis-ci.org/DennisOSRM/Project-OSRM) |
| Project OSRM | develop | [![Build Status](https://travis-ci.org/DennisOSRM/Project-OSRM.png?branch=develop)](https://travis-ci.org/DennisOSRM/Project-OSRM) |
| LUAbind fork | master  | [![Build Status](https://travis-ci.org/DennisOSRM/luabind.png?branch=master)](https://travis-ci.org/DennisOSRM/luabind) |


## Deployment

### Chef

You can use chr4's [osrm cookbook](http://community.opscode.com/cookbooks/osrm) to automatically download, extract and prepare OSRM maps.
For example, to download, extract and prepare the OSRM map for Europe, use this in your recipe

```ruby
include_recipe 'osrm::install_git'

osrm_map 'europe' do
  action :create_if_missing
end
```

Have a look at the [cookbooks readme](https://github.com/chr4-cookbooks/osrm) for more details.
