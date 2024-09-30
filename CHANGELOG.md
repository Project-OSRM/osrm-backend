# Unreleased
  - Changes from 5.27.1
    - Features
      - ADDED: Route pedestrians over highway=platform [#6993](https://github.com/Project-OSRM/osrm-backend/pull/6993)
      - REMOVED: Remove all core-CH left-overs [#6920](https://github.com/Project-OSRM/osrm-backend/pull/6920)
      - ADDED: Add support for a keepalive_timeout flag. [#6674](https://github.com/Project-OSRM/osrm-backend/pull/6674)
      - ADDED: Add support for a default_radius flag. [#6575](https://github.com/Project-OSRM/osrm-backend/pull/6575)
      - ADDED: Add support for disabling feature datasets. [#6666](https://github.com/Project-OSRM/osrm-backend/pull/6666)
      - ADDED: Add support for opposite approach request parameter. [#6842](https://github.com/Project-OSRM/osrm-backend/pull/6842)
      - ADDED: Add support for accessing edge flags in `process_segment` [#6658](https://github.com/Project-OSRM/osrm-backend/pull/6658)
    - Build:
      - CHANGED: Upgrade clang-format to version 15. [#6919](https://github.com/Project-OSRM/osrm-backend/pull/6919)
      - CHANGED: Use Debian Bookworm as base Docker image [#6904](https://github.com/Project-OSRM/osrm-backend/pull/6904)
      - CHANGED: Upgrade CI actions to latest versions [#6893](https://github.com/Project-OSRM/osrm-backend/pull/6893)
      - CHANGED:  Remove outdated warnings #6894 [#6894](https://github.com/Project-OSRM/osrm-backend/pull/6894)
      - ADDED: Add CI job which builds OSRM with gcc 12. [#6455](https://github.com/Project-OSRM/osrm-backend/pull/6455)
      - CHANGED: Upgrade to clang-tidy 15. [#6439](https://github.com/Project-OSRM/osrm-backend/pull/6439)
      - CHANGED: Update actions/cache to v3. [#6420](https://github.com/Project-OSRM/osrm-backend/pull/6420)
      - REMOVED: Drop support of Node 12 & 14. [#6431](https://github.com/Project-OSRM/osrm-backend/pull/6431)
      - ADDED: Add 'load directly' mode to default Cucumber test suite. [#6663](https://github.com/Project-OSRM/osrm-backend/pull/6663)
      - CHANGED: Fix compilation for Boost 1.85.0 [#6856](https://github.com/Project-OSRM/osrm-backend/pull/6856)
      - CHANGED: Drop support for Node 16 [#6855](https://github.com/Project-OSRM/osrm-backend/pull/6855)
      - REMOVED: Remove unused AppVeyor files [#6860](https://github.com/Project-OSRM/osrm-backend/pull/6860)
      - CHANGED: Upgrade clang-format to version 15 [#6859](https://github.com/Project-OSRM/osrm-backend/pull/6859)
    - NodeJS:
      - CHANGED: Use node-api instead of NAN. [#6452](https://github.com/Project-OSRM/osrm-backend/pull/6452)
    - Misc:
      - CHANGED: Use thread_local instead of boost::thread_specific_ptr. [#6991](https://github.com/Project-OSRM/osrm-backend/pull/6991)
      - CHANGED: Bump flatbuffers to v24.3.25 version. [#6988](https://github.com/Project-OSRM/osrm-backend/pull/6988)
      - CHANGED: Add .reserve(...) to assembleGeometry function. [#6983](https://github.com/Project-OSRM/osrm-backend/pull/6983)
      - CHANGED: Get rid of boost::optional leftovers. [#6977](https://github.com/Project-OSRM/osrm-backend/pull/6977)
      - CHANGED: Use Link Time Optimisation whenever possible. [#6967](https://github.com/Project-OSRM/osrm-backend/pull/6967)
      - CHANGED: Use struct instead of tuple to define UnpackedPath. [#6974](https://github.com/Project-OSRM/osrm-backend/pull/6974)
      - CHANGED: Micro performance optimisation in map matching. [#6976](https://github.com/Project-OSRM/osrm-backend/pull/6976)
      - CHANGED: Re-use priority queue in StaticRTree. [#6952](https://github.com/Project-OSRM/osrm-backend/pull/6952)
      - CHANGED: Optimise encodePolyline function. [#6940](https://github.com/Project-OSRM/osrm-backend/pull/6940)
      - CHANGED: Avoid reallocations in base64 encoding. [#6951](https://github.com/Project-OSRM/osrm-backend/pull/6951)
      - CHANGED: Get rid of unused Boost dependencies. [#6960](https://github.com/Project-OSRM/osrm-backend/pull/6960)
      - CHANGED: Apply micro-optimisation for Table & Trip APIs. [#6949](https://github.com/Project-OSRM/osrm-backend/pull/6949)
      - CHANGED: Apply micro-optimisation for Route API. [#6948](https://github.com/Project-OSRM/osrm-backend/pull/6948)
      - CHANGED: Apply micro-optimisation for Match API. [#6945](https://github.com/Project-OSRM/osrm-backend/pull/6945)
      - CHANGED: Apply micro-optimisation for Nearest API. [#6944](https://github.com/Project-OSRM/osrm-backend/pull/6944)
      - CHANGED: Avoid copy of intersection in totalTurnAngle. [#6938](https://github.com/Project-OSRM/osrm-backend/pull/6938)
      - CHANGED: Use std::unordered_map::emplace instead of operator[] when producing JSONs. [#6936](https://github.com/Project-OSRM/osrm-backend/pull/6936)
      - CHANGED: Avoid copy of vectors in MakeRoute function. [#6939](https://github.com/Project-OSRM/osrm-backend/pull/6939)
      - FIXED: Fix bugprone-unused-return-value clang-tidy warning. [#6934](https://github.com/Project-OSRM/osrm-backend/pull/6934)
      - FIXED: Fix performance-noexcept-move-constructor clang-tidy warning. [#6931](https://github.com/Project-OSRM/osrm-backend/pull/6933)
      - FIXED: Fix performance-noexcept-swap clang-tidy warning. [#6931](https://github.com/Project-OSRM/osrm-backend/pull/6931)
      - CHANGED: Use custom struct instead of std::pair in QueryHeap. [#6921](https://github.com/Project-OSRM/osrm-backend/pull/6921)
      - CHANGED: Use std::string_view::starts_with instead of boost::starts_with. [#6918](https://github.com/Project-OSRM/osrm-backend/pull/6918)
      - CHANGED: Get rid of boost::math::constants::* and M_PI in favor of std::numbers. [#6916](https://github.com/Project-OSRM/osrm-backend/pull/6916)
      - CHANGED: Make constants in PackedVector constexpr. [#6917](https://github.com/Project-OSRM/osrm-backend/pull/6917)
      - CHANGED: Use std::variant instead of mapbox::util::variant. [#6903](https://github.com/Project-OSRM/osrm-backend/pull/6903)
      - CHANGED: Bump rapidjson to version f9d53419e912910fd8fa57d5705fa41425428c35 [#6906](https://github.com/Project-OSRM/osrm-backend/pull/6906)
      - CHANGED: Bump mapbox/variant to version 1.2.0 [#6898](https://github.com/Project-OSRM/osrm-backend/pull/6898)
      - CHANGED: Avoid copy of std::function-based callback in path unpacking [#6895](https://github.com/Project-OSRM/osrm-backend/pull/6895)
      - CHANGED: Replace boost::hash by std::hash [#6892](https://github.com/Project-OSRM/osrm-backend/pull/6892)
      - CHANGED: Partial fix migration from boost::optional to std::optional [#6551](https://github.com/Project-OSRM/osrm-backend/issues/6551)
      - CHANGED: Replace boost::filesystem with std::filesystem [#6432](https://github.com/Project-OSRM/osrm-backend/pull/6432)
      - CHANGED: Update Conan Boost version to 1.85.0. [#6868](https://github.com/Project-OSRM/osrm-backend/pull/6868)
      - FIXED: Fix an error in a RouteParameters AnnotationsType operator overload. [#6646](https://github.com/Project-OSRM/osrm-backend/pull/6646)
      - ADDED: Add support for "unlimited" to be passed as a value for the default-radius and max-matching-radius flags. [#6599](https://github.com/Project-OSRM/osrm-backend/pull/6599)
      - CHANGED: Allow -1.0 as unlimited for default_radius value. [#6599](https://github.com/Project-OSRM/osrm-backend/pull/6599)
      - CHANGED: keep libosrm* in the docker image for downstream linking [#6602](https://github.com/Project-OSRM/osrm-backend/pull/6602)
      - CHANGED: Move vector in CSVFilesParser instead copying it. [#6470](https://github.com/Project-OSRM/osrm-backend/pull/6470)
      - REMOVED: Get rid of unused functions in util/json_util.hpp. [#6446](https://github.com/Project-OSRM/osrm-backend/pull/6446)
      - FIXED: Apply workaround for Conan installation issue on CI. [#6442](https://github.com/Project-OSRM/osrm-backend/pull/6442)
      - FIXED: Fix `npm audit` warnings in NPM package. [#6437](https://github.com/Project-OSRM/osrm-backend/pull/6437)
      - FIXED: Handle snapping parameter for all plugins in NodeJs bindings, but not for Route only. [#6417](https://github.com/Project-OSRM/osrm-backend/pull/6417)
      - FIXED: Fix annotations=true handling in NodeJS bindings & libosrm. [#6415](https://github.com/Project-OSRM/osrm-backend/pull/6415/)
      - FIXED: Fix bindings compilation issue on the latest Node. Update NAN to 2.17.0. [#6416](https://github.com/Project-OSRM/osrm-backend/pull/6416)
      - CHANGED: Make edge metrics strongly typed [#6420](https://github.com/Project-OSRM/osrm-backend/pull/6420)
      - FIXED: Typo in file name src/util/timed_historgram.cpp -> src/util/timed_histogram.cpp [#6428](https://github.com/Project-OSRM/osrm-backend/issues/6428)
      - CHANGED: Replace boost::string_ref with std::string_view [#6433](https://github.com/Project-OSRM/osrm-backend/pull/6433)
      - ADDED: Print tracebacks for Lua runtime errors [#6564](https://github.com/Project-OSRM/osrm-backend/pull/6564)
      - FIXED: Added a variable to preprocessor guard in file osrm-backend/include/util/range_table.hpp to solve build error. [#6596](https://github.com/Project-OSRM/osrm-backend/pull/6596)
      - FIXED: Ensure required file check in osrm-routed is correctly enforced. [#6655](https://github.com/Project-OSRM/osrm-backend/pull/6655)
      - FIXED: Correct HTTP docs to reflect summary output dependency on steps parameter. [#6655](https://github.com/Project-OSRM/osrm-backend/pull/6655)
      - ADDED: Extract prerelease/build information from package semver [#6839](https://github.com/Project-OSRM/osrm-backend/pull/6839)
    - Profiles:
      - FIXED: Bicycle and foot profiles now don't route on proposed ways [#6615](https://github.com/Project-OSRM/osrm-backend/pull/6615)
      - ADDED: Add optional support of cargo bike exclusion and width to bicyle profile [#7044](https://github.com/Project-OSRM/osrm-backend/pull/7044)
    - Routing:
      - FIXED: Fix adding traffic signal penalties during compression [#6419](https://github.com/Project-OSRM/osrm-backend/pull/6419)
      - FIXED: Correctly handle compressed traffic signals. [#6724](https://github.com/Project-OSRM/osrm-backend/pull/6724)
      - FIXED: Fix bug when searching for maneuver overrides [#6739](https://github.com/Project-OSRM/osrm-backend/pull/6739)
      - FIXED: Remove force-loop checks for routes with u-turns [#6858](https://github.com/Project-OSRM/osrm-backend/pull/6858)
      - FIXED: Correctly check runtime search conditions for forcing routing steps [#6866](https://github.com/Project-OSRM/osrm-backend/pull/6866)
    - Map Matching:
      - CHANGED: Optimise path distance calculation in MLD map matching even more. [#6884](https://github.com/Project-OSRM/osrm-backend/pull/6884)
      - CHANGED: Optimise path distance calculation in MLD map matching. [#6876](https://github.com/Project-OSRM/osrm-backend/pull/6876)
      - CHANGED: Optimise R-tree queries in the case of map matching. [#6881](https://github.com/Project-OSRM/osrm-backend/pull/6876)
    - Debug tiles:
      - FIXED: Ensure speed layer features have unique ids. [#6726](https://github.com/Project-OSRM/osrm-backend/pull/6726)

# 5.27.1
  - Changes from 5.27.0
    - Misc:
      - FIXED: Revert back to using custom HTTP parser instead of Boost.Beast. [#6407](https://github.com/Project-OSRM/osrm-backend/pull/6407)
      - FIXED: Fix bug with large HTTP requests leading to Bad Request in osrm-routed. [#6403](https://github.com/Project-OSRM/osrm-backend/pull/6403)
    - Routing:
      - CHANGED: Add support for surface=metal,grass_paver,woodchips in bicyle profile. [#6395](https://github.com/Project-OSRM/osrm-backend/pull/6395)

# 5.27.0
  - Changes from 5.26.0
    - API:
      - ADDED: Add Flatbuffers support to NodeJS bindings. [#6338](https://github.com/Project-OSRM/osrm-backend/pull/6338)
      - CHANGED: Add `data_version` field to responses of all services. [#5387](https://github.com/Project-OSRM/osrm-backend/pull/5387)
      - FIXED: Use Boost.Beast to parse HTTP request. [#6294](https://github.com/Project-OSRM/osrm-backend/pull/6294)
      - FIXED: Fix inefficient osrm-routed connection handling [#6113](https://github.com/Project-OSRM/osrm-backend/pull/6113)
      - FIXED: Fix HTTP compression precedence [#6113](https://github.com/Project-OSRM/osrm-backend/pull/6113)
    - NodeJS:
      - FIXED: Support `skip_waypoints` in Node bindings [#6060](https://github.com/Project-OSRM/osrm-backend/pull/6060)
    - Misc:
      - ADDED: conanbuildinfo.json for easy reading of dependencies [#6388](https://github.com/Project-OSRM/osrm-backend/pull/6388)
      - CHANGED: Improve performance of JSON rendering. Fix undefined behaviour in JSON numbers formatting. [#6380](https://github.com/Project-OSRM/osrm-backend/pull/6380)
      - ADDED: Add timestamps for logs. [#6375](https://github.com/Project-OSRM/osrm-backend/pull/6375)
      - CHANGED: Improve performance of map matching via getPathDistance optimization. [#6378](https://github.com/Project-OSRM/osrm-backend/pull/6378)
      - CHANGED: Optimize RestrictionParser performance. [#6344](https://github.com/Project-OSRM/osrm-backend/pull/6344)
      - ADDED: Support floats for speed value in traffic updates CSV. [#6327](https://github.com/Project-OSRM/osrm-backend/pull/6327)
      - CHANGED: Use Lua 5.4 in Docker image. [#6346](https://github.com/Project-OSRM/osrm-backend/pull/6346)
      - CHANGED: Remove redundant nullptr check. [#6326](https://github.com/Project-OSRM/osrm-backend/pull/6326)
      - CHANGED: missing files list is included in exception message. [#5360](https://github.com/Project-OSRM/osrm-backend/pull/5360)
      - CHANGED: Do not use deprecated Callback::Call overload in Node bindings. [#6318](https://github.com/Project-OSRM/osrm-backend/pull/6318)
      - FIXED: Fix distance calculation consistency. [#6315](https://github.com/Project-OSRM/osrm-backend/pull/6315)
      - FIXED: Fix performance issue after migration to sol2 3.3.0. [#6304](https://github.com/Project-OSRM/osrm-backend/pull/6304)
      - CHANGED: Pass osm_node_ids by reference in osrm::updater::Updater class. [#6298](https://github.com/Project-OSRM/osrm-backend/pull/6298)
      - FIXED: Fix bug with reading Set values from Lua scripts. [#6285](https://github.com/Project-OSRM/osrm-backend/pull/6285)
      - FIXED: Bug in bicycle profile that caused exceptions if there is a highway=bicycle in the data. [#6296](https://github.com/Project-OSRM/osrm-backend/pull/6296)
      - FIXED: Internal refactoring of identifier types used in data facade [#6044](https://github.com/Project-OSRM/osrm-backend/pull/6044)
      - CHANGED: Update docs to reflect recent build and dependency changes [#6383](https://github.com/Project-OSRM/osrm-backend/issues/6383)
    - Build:
      - REMOVED: Get rid of Mason. [#6387](https://github.com/Project-OSRM/osrm-backend/pull/6387)
      - CHANGED: Use clang-format from CI base image. [#6391](https://github.com/Project-OSRM/osrm-backend/pull/6391)
      - ADDED: Build Node bindings on Windows. [#6334](https://github.com/Project-OSRM/osrm-backend/pull/6334)
      - ADDED: Configure cross-compilation for Apple Silicon. [#6360](https://github.com/Project-OSRM/osrm-backend/pull/6360)
      - CHANGED: Use apt-get to install Clang on CI. [#6345](https://github.com/Project-OSRM/osrm-backend/pull/6345)
      - CHANGED: Fix TBB in case of Conan + NodeJS build. [#6333](https://github.com/Project-OSRM/osrm-backend/pull/6333)
      - CHANGED: Migrate to modern TBB version. [#6300](https://github.com/Project-OSRM/osrm-backend/pull/6300)
      - CHANGED: Enable performance-move-const-arg clang-tidy check. [#6319](https://github.com/Project-OSRM/osrm-backend/pull/6319)
      - CHANGED: Use the latest node on CI. [#6317](https://github.com/Project-OSRM/osrm-backend/pull/6317)
      - CHANGED: Migrate Windows CI to GitHub Actions. [#6312](https://github.com/Project-OSRM/osrm-backend/pull/6312)
      - ADDED: Add smoke test for Docker image. [#6313](https://github.com/Project-OSRM/osrm-backend/pull/6313)
      - CHANGED: Update libosmium to version 2.18.0. [#6303](https://github.com/Project-OSRM/osrm-backend/pull/6303)
      - CHANGED: Remove EXACT from find_package if using Conan. [#6299](https://github.com/Project-OSRM/osrm-backend/pull/6299)
      - CHANGED: Configure Undefined Behaviour Sanitizer. [#6290](https://github.com/Project-OSRM/osrm-backend/pull/6290)
      - CHANGED: Use Conan instead of Mason to install code dependencies. [#6284](https://github.com/Project-OSRM/osrm-backend/pull/6284)
      - CHANGED: Migrate to C++17. Update sol2 to 3.3.0. [#6279](https://github.com/Project-OSRM/osrm-backend/pull/6279)
      - CHANGED: Update macOS CI image to macos-11. [#6286](https://github.com/Project-OSRM/osrm-backend/pull/6286)
      - CHANGED: Enable even more clang-tidy checks. [#6273](https://github.com/Project-OSRM/osrm-backend/pull/6273)
      - CHANGED: Configure CMake to not build flatbuffers tests and samples. [#6274](https://github.com/Project-OSRM/osrm-backend/pull/6274)
      - CHANGED: Enable more clang-tidy checks. [#6270](https://github.com/Project-OSRM/osrm-backend/pull/6270)
      - CHANGED: Configure clang-tidy job on CI. [#6261](https://github.com/Project-OSRM/osrm-backend/pull/6261)
      - CHANGED: Use Github Actions for building container images [#6138](https://github.com/Project-OSRM/osrm-backend/pull/6138)
      - CHANGED: Upgrade Boost dependency to 1.70 [#6113](https://github.com/Project-OSRM/osrm-backend/pull/6113)
      - CHANGED: Upgrade Ubuntu CI builds to 20.04  [#6119](https://github.com/Project-OSRM/osrm-backend/pull/6119)
      - CHANGED: Make building osrm-routed optional [#6144](https://github.com/Project-OSRM/osrm-backend/pull/6144)
      - FIXED: Run all unit tests in CI [#5248](https://github.com/Project-OSRM/osrm-backend/pull/5248)
      - FIXED: Fix installation of Mason CMake and 32 bit CI build [#6170](https://github.com/Project-OSRM/osrm-backend/pull/6170)
      - FIXED: Fixed Node docs generation check in CI. [#6058](https://github.com/Project-OSRM/osrm-backend/pull/6058)
      - CHANGED: Docker build, enabled arm64 build layer [#6172](https://github.com/Project-OSRM/osrm-backend/pull/6172)
      - CHANGED: Docker build, enabled apt-get update/install caching in separate layer for build phase [#6175](https://github.com/Project-OSRM/osrm-backend/pull/6175)
      - FIXED: Bump CI complete meta job to ubuntu-20.04 [#6323](https://github.com/Project-OSRM/osrm-backend/pull/6323)
      - CHANGED: Node packages are now scoped by @project-osrm [#6386](https://github.com/Project-OSRM/osrm-backend/issues/6386)
    - Routing:
      - CHANGED: Lazily generate optional route path data [#6045](https://github.com/Project-OSRM/osrm-backend/pull/6045)
      - FIXED: Completed support for no_entry and no_exit turn restrictions. [#5988](https://github.com/Project-OSRM/osrm-backend/pull/5988)
      - ADDED: Add support for non-round-trips with a single fixed endpoint. [#6050](https://github.com/Project-OSRM/osrm-backend/pull/6050)
      - FIXED: Improvements to maneuver override processing [#6125](https://github.com/Project-OSRM/osrm-backend/pull/6125)
      - ADDED: Support snapping to multiple ways at an input location. [#5953](https://github.com/Project-OSRM/osrm-backend/pull/5953)
      - FIXED: Fix snapping target locations to ways used in turn restrictions. [#6339](https://github.com/Project-OSRM/osrm-backend/pull/6339)
      - ADDED: Support OSM traffic signal directions. [#6153](https://github.com/Project-OSRM/osrm-backend/pull/6153)
      - FIXED: Ensure u-turn exists in intersection view. [#6376](https://github.com/Project-OSRM/osrm-backend/pull/6376)
      - FIXED: Gracefully handle no-turn intersections in guidance processing. [#6382](https://github.com/Project-OSRM/osrm-backend/issues/6382)
    - Profile:
      - CHANGED: Bicycle surface speeds [#6212](https://github.com/Project-OSRM/osrm-backend/pull/6212)
    - Tools:
      - CHANGED: Do not generate intermediate .osrm file in osrm-extract. [#6354](https://github.com/Project-OSRM/osrm-backend/pull/6354)

# 5.26.0
  - Changes from 5.25.0
    - API:
      - FIXED: Allow for special characters in the profile/method as part of the HTTP URL. [#6090](https://github.com/Project-OSRM/osrm-backend/pull/6090)
      - FIXED: Set osrm-routed to immediately close bad connections [#6112](https://github.com/Project-OSRM/osrm-backend/pull/6112)
    - Build:
      - CHANGED: Replace Travis with Github Actions for CI builds [#6071](https://github.com/Project-OSRM/osrm-backend/pull/6071)
      - FIXED: Fixed Boost link flags in pkg-config file. [#6083](https://github.com/Project-OSRM/osrm-backend/pull/6083)
      - FIXED: Fixed test cache to consider MLD executable changes. [#6129](https://github.com/Project-OSRM/osrm-backend/pull/6129)
    - Routing:
      - FIXED: Fix generation of inefficient MLD partitions [#6084](https://github.com/Project-OSRM/osrm-backend/pull/6084)
      - FIXED: Fix MLD level mask generation to support 64-bit masks. [#6123](https://github.com/Project-OSRM/osrm-backend/pull/6123)
      - FIXED: Fix metric offset overflow for large MLD partitions. This breaks the **data format** [#6124](https://github.com/Project-OSRM/osrm-backend/pull/6124)

# 5.25.0
  - Changes from 5.24.0
    - Build:
      - CHANGED: Node binaries now use Github Releases for hosting [#6030](https://github.com/Project-OSRM/osrm-backend/pull/6030)
    - Misc:
      - FIXED: Upgrade to @mapbox/node-pre-gyp fix various bugs with Node 12/14 [#5991](https://github.com/Project-OSRM/osrm-backend/pull/5991)
      - FIXED: `valid` type in documentation examples [#5990](https://github.com/Project-OSRM/osrm-backend/issues/5990)
      - FIXED: Remove redundant loading of .osrm.cell_metrics [#6019](https://github.com/Project-OSRM/osrm-backend/issues/6019)
      - CHANGED: Increase PackedOSMIDs size to 34 bits. This breaks the **data format** [#6020](https://github.com/Project-OSRM/osrm-backend/issues/6020)
    - Profile:
      - FIXED: Add kerb barrier exception to default car profile. [#5999](https://github.com/Project-OSRM/osrm-backend/pull/5999)

# 5.24.0
  - Changes from 5.23.0
    - Features
      - ADDED: Added support for multiple via-way restrictions. [#5907](https://github.com/Project-OSRM/osrm-backend/pull/5907)
      - ADDED: Add node bindings support for Node 12, 14, and publish binaries [#5918](https://github.com/Project-OSRM/osrm-backend/pull/5918)
      - REMOVED: we no longer publish Node 8 binary modules (they are still buildable from source) [#5918](https://github.com/Project-OSRM/osrm-backend/pull/5918)
    - Routing:
      - FIXED: Avoid copying ManyToMany table results [#5923](https://github.com/Project-OSRM/osrm-backend/pull/5923)
      - FIXED: Reduce copying in API parameter constructors [#5925](https://github.com/Project-OSRM/osrm-backend/pull/5925)
    - Misc:
      - CHANGED: Cleanup NodeJS dependencies [#5945](https://github.com/Project-OSRM/osrm-backend/pull/5945)
      - CHANGED: Unify `.osrm.turn_penalites_index` dump processing same with `.osrm.turn_weight_penalties` and `.osrm.turn_duration_penalties` [#5868](https://github.com/Project-OSRM/osrm-backend/pull/5868)
      - FIXED: Properly validate source/destination validation in NodeJS table service [#5595](https://github.com/Project-OSRM/osrm-backend/pull/5595/files)
      - FIXED: turn.roads_on_the_left not containing incoming roads and turn.roads_on_the_right not containing outgoing roads on two-way roads [#5128](https://github.com/Project-OSRM/osrm-backend/issues/5128)
    - Profile:
      - ADDED: Profile debug script which fetches a way from OSM then outputs the result of the profile. [#5908](https://github.com/Project-OSRM/osrm-backend/pull/5908)
    - Infrastructure
      - CHANGED: Bundled protozero updated to v1.7.0. [#5858](https://github.com/Project-OSRM/osrm-backend/pull/5858)
    - Windows:
      - FIXED: Fix bit-shift overflow in MLD partition step. [#5878](https://github.com/Project-OSRM/osrm-backend/pull/5878)
      - FIXED: Fix vector bool permutation in graph contraction step [#5882](https://github.com/Project-OSRM/osrm-backend/pull/5882)
    - API:
      - FIXED: Undo libosrm API break by adding old interface as method overload [#5861](https://github.com/Project-OSRM/osrm-backend/pull/5861)
      - FIXED: Fixed validation of sources/destinations when accessed via node bindings [#5595](https://github.com/Project-OSRM/osrm-backend/pull/5595)

# 5.23.0
  - Changes from 5.22.0
    - Build:
      - FIXED: pessimistic calls to std::move [#5560](https://github.com/Project-OSRM/osrm-backend/pull/5561)
    - Features:
      - ADDED: new API parameter - `snapping=any|default` to allow snapping to previously unsnappable edges [#5361](https://github.com/Project-OSRM/osrm-backend/pull/5361)
      - ADDED: keepalive support to the osrm-routed HTTP server [#5518](https://github.com/Project-OSRM/osrm-backend/pull/5518)
      - ADDED: flatbuffers output format support [#5513](https://github.com/Project-OSRM/osrm-backend/pull/5513)
      - ADDED: Global 'skip_waypoints' option [#5556](https://github.com/Project-OSRM/osrm-backend/pull/5556)
      - FIXED: Install the libosrm_guidance library correctly [#5604](https://github.com/Project-OSRM/osrm-backend/pull/5604)
      - FIXED: Http Handler can now deal witch optional whitespace between header-key and -value [#5606](https://github.com/Project-OSRM/osrm-backend/issues/5606)
    - Routing:
      - CHANGED: allow routing past `barrier=arch` [#5352](https://github.com/Project-OSRM/osrm-backend/pull/5352)
      - CHANGED: default car weight was reduced to 2000 kg. [#5371](https://github.com/Project-OSRM/osrm-backend/pull/5371)
      - CHANGED: default car height was reduced to 2 meters. [#5389](https://github.com/Project-OSRM/osrm-backend/pull/5389)
      - FIXED: treat `bicycle=use_sidepath` as no access on the tagged way. [#5622](https://github.com/Project-OSRM/osrm-backend/pull/5622)
      - FIXED: fix table result when source and destination on same one-way segment. [#5828](https://github.com/Project-OSRM/osrm-backend/pull/5828)
      - FIXED: fix occasional segfault when swapping data with osrm-datastore and using `exclude=` [#5844](https://github.com/Project-OSRM/osrm-backend/pull/5844)
      - FIXED: fix crash in MLD alternative search if source or target are invalid [#5851](https://github.com/Project-OSRM/osrm-backend/pull/5851)
    - Misc:
      - CHANGED: Reduce memory usage for raster source handling. [#5572](https://github.com/Project-OSRM/osrm-backend/pull/5572)
      - CHANGED: Add cmake option `ENABLE_DEBUG_LOGGING` to control whether output debug logging. [#3427](https://github.com/Project-OSRM/osrm-backend/issues/3427)
      - CHANGED: updated extent of Hong Kong as left hand drive country. [#5535](https://github.com/Project-OSRM/osrm-backend/issues/5535)
      - FIXED: corrected error message when failing to snap input coordinates [#5846](https://github.com/Project-OSRM/osrm-backend/pull/5846)
    - Infrastructure
      - REMOVED: STXXL support removed as STXXL became abandonware. [#5760](https://github.com/Project-OSRM/osrm-backend/pull/5760)

# 5.22.0
  - Changes from 5.21.0
    - Build:
      - ADDED: optionally build Node `lts` and `latest` bindings [#5347](https://github.com/Project-OSRM/osrm-backend/pull/5347)
    - Features:
      - ADDED: new waypoints parameter to the `route` plugin, enabling silent waypoints [#5345](https://github.com/Project-OSRM/osrm-backend/pull/5345)
      - ADDED: data timestamp information in the response (saved in new file `.osrm.timestamp`). [#5115](https://github.com/Project-OSRM/osrm-backend/issues/5115)

# 5.21.0
  - Changes from 5.20.0
    - Features:
      - ADDED: all waypoints in responses now contain a distance property between the original coordinate and the snapped location. [#5255](https://github.com/Project-OSRM/osrm-backend/pull/5255)
      - ADDED: if `fallback_speed` is used, a new structure `fallback_speed_cells` will describe which cells contain estimated values [#5259](https://github.com/Project-OSRM/osrm-backend/pull/5259)
      - REMOVED: we no longer publish Node 4 or 6 binary modules (they are still buildable from source) [#5314](https://github.com/Project-OSRM/osrm-backend/pull/5314)
    - Table:
      - ADDED: new parameter `scale_factor` which will scale the cell `duration` values by this factor. [#5298](https://github.com/Project-OSRM/osrm-backend/pull/5298)
      - FIXED: only trigger `scale_factor` code to scan matrix when necessary. [#5303](https://github.com/Project-OSRM/osrm-backend/pull/5303)
      - FIXED: fix bug in reverse offset calculation that sometimes lead to negative (and other incorrect) values in distance table results [#5315](https://github.com/Project-OSRM/osrm-backend/pull/5315)
    - Docker:
      - FIXED: use consistent boost version between build and runtime [#5311](https://github.com/Project-OSRM/osrm-backend/pull/5311)
      - FIXED: don't override default permissions on /opt [#5311](https://github.com/Project-OSRM/osrm-backend/pull/5311)
    - Matching:
      - CHANGED: matching will now consider edges marked with is_startpoint=false, allowing matching over ferries and other previously non-matchable edge types. [#5297](https://github.com/Project-OSRM/osrm-backend/pull/5297)
    - Profile:
      - ADDED: Parse `source:maxspeed` and `maxspeed:type` tags to apply maxspeeds and add belgian flanders rural speed limit. [#5217](https://github.com/Project-OSRM/osrm-backend/pull/5217)
      - CHANGED: Refactor maxspeed parsing to use common library. [#5144](https://github.com/Project-OSRM/osrm-backend/pull/5144)

# 5.20.0
  - Changes from 5.19.0:
    - Table:
      - CHANGED: switch to pre-calculated distances for table responses for large speedup and 10% memory increase. [#5251](https://github.com/Project-OSRM/osrm-backend/pull/5251)
      - ADDED: new parameter `fallback_speed` which will fill `null` cells with estimated value [#5257](https://github.com/Project-OSRM/osrm-backend/pull/5257)
      - CHANGED: Remove API check for matrix sources/destination length to be less than or equal to coordinates length. [#5298](https://github.com/Project-OSRM/osrm-backend/pull/5289)
      - FIXED: Fix crashing bug when using fallback_speed parameter with more sources than destinations. [#5291](https://github.com/Project-OSRM/osrm-backend/pull/5291)
    - Features:
      - ADDED: direct mmapping of datafiles is now supported via the `--mmap` switch. [#5242](https://github.com/Project-OSRM/osrm-backend/pull/5242)
      - REMOVED: the previous `--memory_file` switch is now deprecated and will fallback to `--mmap` [#5242](https://github.com/Project-OSRM/osrm-backend/pull/5242)
      - ADDED: Now publishing Node 10.x LTS binary modules [#5246](https://github.com/Project-OSRM/osrm-backend/pull/5246)
    - Windows:
      - FIXED: Windows builds again. [#5249](https://github.com/Project-OSRM/osrm-backend/pull/5249)
    - Docker:
      - CHANGED: switch from Alpine Linux to Debian Buster base images [#5281](https://github.com/Project-OSRM/osrm-backend/pull/5281)

# 5.19.0
  - Changes from 5.18.0:
    - Optimizations:
      - CHANGED: Use Grisu2 for serializing floating point numbers. [#5188](https://github.com/Project-OSRM/osrm-backend/pull/5188)
      - ADDED: Node bindings can return pre-rendered JSON buffer. [#5189](https://github.com/Project-OSRM/osrm-backend/pull/5189)
    - Profiles:
      - CHANGED: Bicycle profile now blacklists barriers instead of whitelisting them [#5076
](https://github.com/Project-OSRM/osrm-backend/pull/5076/)
      - CHANGED: Foot profile now blacklists barriers instead of whitelisting them [#5077
](https://github.com/Project-OSRM/osrm-backend/pull/5077/)
      - CHANGED: Support maxlength and maxweight in car profile [#5101](https://github.com/Project-OSRM/osrm-backend/pull/5101)
    - Bugfixes:
      - FIXED: collapsing of ExitRoundabout instructions [#5114](https://github.com/Project-OSRM/osrm-backend/issues/5114)
    - Misc:
      - CHANGED: Support up to 512 named shared memory regions [#5185](https://github.com/Project-OSRM/osrm-backend/pull/5185)

# 5.18.0
  - Changes from 5.17.0:
    - Features:
      - ADDED: `table` plugin now optionally returns `distance` matrix as part of response [#4990](https://github.com/Project-OSRM/osrm-backend/pull/4990)
      - ADDED: New optional parameter `annotations` for `table` that accepts `distance`, `duration`, or both `distance,duration` as values [#4990](https://github.com/Project-OSRM/osrm-backend/pull/4990)
    - Infrastructure:
      - ADDED: Updated libosmium and added protozero and vtzero libraries [#5037](https://github.com/Project-OSRM/osrm-backend/pull/5037)
      - CHANGED: Use vtzero library in tile plugin [#4686](https://github.com/Project-OSRM/osrm-backend/pull/4686)
    - Profile:
      - ADDED: Bicycle profile now returns classes for ferry and tunnel routes. [#5054](https://github.com/Project-OSRM/osrm-backend/pull/5054)
      - ADDED: Bicycle profile allows to exclude ferry routes (default to not enabled) [#5054](https://github.com/Project-OSRM/osrm-backend/pull/5054)

# 5.17.1
  - Changes from 5.17.0:
    - Bugfixes:
      - FIXED: Do not combine a segregated edge with a roundabout [#5039](https://github.com/Project-OSRM/osrm-backend/issues/5039)

# 5.17.0
  - Changes from 5.16.0:
    - Bugfixes:
      - FIXED: deduplication of route steps when waypoints are used [#4909](https://github.com/Project-OSRM/osrm-backend/issues/4909)
      - FIXED: Use smaller range for U-turn angles in map-matching [#4920](https://github.com/Project-OSRM/osrm-backend/pull/4920)
      - FIXED: Remove the last short annotation segment in `trimShortSegments` [#4946](https://github.com/Project-OSRM/osrm-backend/pull/4946)
      - FIXED: Properly calculate annotations for speeds, durations and distances when waypoints are used with mapmatching [#4949](https://github.com/Project-OSRM/osrm-backend/pull/4949)
      - FIXED: Don't apply unimplemented SH and PH conditions in OpeningHours and add inversed date ranges [#4992](https://github.com/Project-OSRM/osrm-backend/issues/4992)
      - FIXED: integer overflow in `DynamicGraph::Renumber` [#5021](https://github.com/Project-OSRM/osrm-backend/pull/5021)
    - Profile:
      - CHANGED: Handle oneways in get_forward_backward_by_key [#4929](https://github.com/Project-OSRM/osrm-backend/pull/4929)
      - FIXED: Do not route against oneway road if there is a cycleway in the wrong direction; also review bike profile [#4943](https://github.com/Project-OSRM/osrm-backend/issues/4943)
      - CHANGED: Make cyclability weighting of the bike profile prefer safer routes more strongly [#5015](https://github.com/Project-OSRM/osrm-backend/issues/5015)
    - Guidance:
      - CHANGED: Don't use obviousness for links bifurcations [#4929](https://github.com/Project-OSRM/osrm-backend/pull/4929)
      - FIXED: Adjust Straight direction modifiers of side roads in driveway handler [#4929](https://github.com/Project-OSRM/osrm-backend/pull/4929)
      - CHANGED: Added post process logic to collapse segregated turn instructions [#4925](https://github.com/Project-OSRM/osrm-backend/pull/4925)
      - ADDED: Maneuver relation now supports `straight` as a direction [#4995](https://github.com/Project-OSRM/osrm-backend/pull/4995)
      - FIXED: Support spelling maneuver relation with British spelling [#4950](https://github.com/Project-OSRM/osrm-backend/issues/4950)
    - Tools:
      - ADDED: `osrm-routed` accepts a new property `--memory_file` to store memory in a file on disk. [#4881](https://github.com/Project-OSRM/osrm-backend/pull/4881)
      - ADDED: `osrm-datastore` accepts a new parameter `--dataset-name` to select the name of the dataset. [#4982](https://github.com/Project-OSRM/osrm-backend/pull/4982)
      - ADDED: `osrm-datastore` accepts a new parameter `--list` to list all datasets loaded into memory. [#4982](https://github.com/Project-OSRM/osrm-backend/pull/4982)
      - ADDED: `osrm-datastore` accepts a new parameter `--only-metric` to only reload the data that can be updated by a weight update (reduces memory for traffic updates). [#5002](https://github.com/Project-OSRM/osrm-backend/pull/5002)
      - ADDED: `osrm-routed` accepts a new parameter `--dataset-name` to select the shared-memory dataset to use. [#4982](https://github.com/Project-OSRM/osrm-backend/pull/4982)
    - NodeJS:
      - ADDED: `OSRM` object accepts a new option `memory_file` that stores the memory in a file on disk. [#4881](https://github.com/Project-OSRM/osrm-backend/pull/4881)
      - ADDED: `OSRM` object accepts a new option `dataset_name` to select the shared-memory dataset. [#4982](https://github.com/Project-OSRM/osrm-backend/pull/4982)
    - Internals
      - CHANGED: Updated segregated intersection identification [#4845](https://github.com/Project-OSRM/osrm-backend/pull/4845) [#4968](https://github.com/Project-OSRM/osrm-backend/pull/4968)
      - REMOVED: Remove `.timestamp` file since it was unused [#4960](https://github.com/Project-OSRM/osrm-backend/pull/4960)
    - Documentation:
      - ADDED: Add documentation about OSM node ids in nearest service response [#4436](https://github.com/Project-OSRM/osrm-backend/pull/4436)
    - Performance
      - FIXED: Speed up response time when lots of legs exist and geojson is used with `steps=true` [#4936](https://github.com/Project-OSRM/osrm-backend/pull/4936)
      - FIXED: Return iterators instead of vectors in datafacade_base functions [#4969](https://github.com/Project-OSRM/osrm-backend/issues/4969)
    - Misc:
      - ADDED: expose name for datasource annotations as metadata [#4973](https://github.com/Project-OSRM/osrm-backend/pull/4973)

# 5.16.0
  - Changes from 5.15.2:
    - Guidance
      - ADDED #4676: Support for maneuver override relation, allowing data-driven overrides for turn-by-turn instructions [#4676](https://github.com/Project-OSRM/osrm-backend/pull/4676)
      - CHANGED #4830: Announce reference change if names are empty
      - CHANGED #4835: MAXIMAL_ALLOWED_SEPARATION_WIDTH increased to 12 meters
      - CHANGED #4842: Lower priority links from a motorway now are used as motorway links [#4842](https://github.com/Project-OSRM/osrm-backend/pull/4842)
      - CHANGED #4895: Use ramp bifurcations as fork intersections [#4895](https://github.com/Project-OSRM/osrm-backend/issues/4895)
      - CHANGED #4893: Handle motorway forks with links as normal motorway intersections[#4893](https://github.com/Project-OSRM/osrm-backend/issues/4893)
      - FIXED #4905: Check required tags of `maneuver` relations [#4905](https://github.com/Project-OSRM/osrm-backend/pull/4905)
    - Profile:
      - FIXED: `highway=service` will now be used for restricted access, `access=private` is still disabled for snapping.
      - ADDED #4775: Exposes more information to the turn function, now being able to set turn weights with highway and access information of the turn as well as other roads at the intersection [#4775](https://github.com/Project-OSRM/osrm-backend/issues/4775)
      - FIXED #4763: Add support for non-numerical units in car profile for maxheight [#4763](https://github.com/Project-OSRM/osrm-backend/issues/4763)
      - ADDED #4872: Handling of `barrier=height_restrictor` nodes [#4872](https://github.com/Project-OSRM/osrm-backend/pull/4872)

# 5.15.2
  - Changes from 5.15.1:
    - Features:
        - ADDED: Exposed the waypoints parameter in the node bindings interface
    - Bugfixes:
        - FIXED: Segfault causing bug in leg collapsing map matching when traversing edges in reverse

# 5.15.1
  - Changes from 5.15.0:
    - Bugfixes:
      - FIXED: Segfault in map matching when RouteLeg collapsing code is run on a match with multiple submatches
    - Guidance:
      - Set type of trivial intersections where classes change to Suppressed instead of NoTurn

# 5.15.0
  - Changes from 5.14.3:
    - Bugfixes:
      - FIXED #4704: Fixed regression in bearings reordering introduced in 5.13 [#4704](https://github.com/Project-OSRM/osrm-backend/issues/4704)
      - FIXED #4781: Fixed overflow exceptions in percent-encoding parsing
      - FIXED #4770: Fixed exclude flags for single toll road scenario
      - FIXED #4283: Fix overflow on zero duration segments
      - FIXED #4804: Ignore no_*_on_red turn restrictions
    - Guidance:
      - CHANGED #4706: Guidance refactoring step to decouple intersection connectivity analysis and turn instructions generation [#4706](https://github.com/Project-OSRM/osrm-backend/pull/4706)
      - CHANGED #3491: Refactor `isThroughStreet`/Intersection options
    - Profile:
      - ADDED: `tunnel` as a new class in car profile so that sections of the route with tunnel tags will be marked as such

# 5.14.3
  - Changes from 5.14.2:
    - Features:
      - Added a `waypoints` parameter to the match service plugin that accepts indices to input coordinates and treats only those points as waypoints in the response format.
    - Bugfixes:
      - FIXED #4754: U-Turn penalties are applied to straight turns.
      - FIXED #4756: Removed too restrictive road name check in the sliproad handler
      - FIXED #4731: Use correct weights for edge-based graph duplicated via nodes.
    - Profile:
      - CHANGED: added Belarus speed limits
      - CHANGED: set default urban speed in Ukraine to 50kmh

# 5.14.2
  - Changes from 5.14.1:
    - Bugfixes:
      - FIXED #4727: Erroring when a old .core file is present.
      - FIXED #4642: Update checks for EMPTY_NAMEID to check for empty name strings
      - FIXED #4738: Fix potential segmentation fault
    - Node.js Bindings:
      - ADDED: Exposed new `max_radiuses_map_matching` option from `EngingConfig` options
    - Tools:
      - ADDED: New osrm-routed `max_radiuses_map_matching` command line flag to optionally set a maximum radius for map matching

# 5.14.1
  - Changes from 5.14.0
    - Bugfixes:
      - FIXED: don't use removed alternative candidates in `filterPackedPathsByCellSharing`

# 5.14.0
  - Changes from 5.13
    - API:
      - ADDED: new RouteStep property `driving_side` that has either "left" or "right" for that step
    - Misc:
      - ADDED: Bundles a rough (please improve!) driving-side GeoJSON file for use with `osrm-extract --location-dependent-data data/driving_side.geojson`
      - CHANGED: Conditional turn parsing is disabled by default now
      - ADDED: Adds a tool to analyze turn instruction generation in a dataset.  Useful for tracking turn-by-turn heuristic changes over time.
      - CHANGED: Internal refactoring of guidance code as a first step towards a re-runnable guidance pipeline
      - ADDED: Now publishing Node 8.x LTS binary modules
    - Profile:
      - CHANGED: Remove dependency on turn types and turn modifier in the process_turn function in the `car.lua` profile. Guidance instruction types are not used to influence turn penalty anymore so this will break backward compatibility between profile version 3 and 4.
    - Guidance:
      - ADDED: New internal flag on "segregated intersections" - in the future, will should allow collapsing of instructions across complex intersection geometry where humans only perceive a single maneuver
      - CHANGED: Decrease roundabout turn radius threshold from 25m to 15m - adds some "exit the roundabout" instructions for moderately sized roundabouts that were being missed previously
    - Docker:
      - CHANGED: switch to alpine 3.6, and use a multistage build to reduce image size
    - Build:
      - FIX: use LUA_LIBRARY_DIRS to propertly detect Lua on all platforms
    - Docs:
      - FIX: clarify description of roundabout exit instructions
    - Bugfixes:
      - FIXED: Fix bug where merge instructions got the wrong direction modifier ([PR #4670](https://github.com/Project-OSRM/osrm-backend/pull/4670))
      - FIXED: Properly use the `profile.properties.left_hand_driving` property, there was a typo that meant it had no effect
      - FIXED: undefined behaviour when alternative candidate via node is same as source node ([#4691](https://github.com/Project-OSRM/osrm-backend/issues/4691))
      - FIXED: ensure libosrm.pc is pushed to the correct location for pkgconfig to find it on all platforms
      - FIXED: don't consider empty names + empty refs as a valid name for u-turns

# 5.13.0
  - Changes from 5.12:
    - Profile:
      - Append cardinal directions from route relations to ref fields to improve instructions; off by default see `profile.cardinal_directions`
      - Support of `distance` weight in foot and bicycle profiles
      - Support of relations processing
      - Added `way:get_location_tag(key)` method to get location-dependent tags https://github.com/Project-OSRM/osrm-backend/wiki/Using-location-dependent-data-in-profiles
      - Added `forward_ref` and `backward_ref` support
      - Left-side driving mode is specified by a local Boolean flag `is_left_hand_driving` in `ExtractionWay` and `ExtractionTurn`
      - Support literal values for maxspeeds in NO, PL and ZA
    - Infrastructure:
      - Lua 5.1 support is removed due to lack of support in sol2 https://github.com/ThePhD/sol2/issues/302
      - Fixed pkg-config version of OSRM
      - Removed `.osrm.core` file since CoreCH is deprecated now.
    - Tools:
      - Because of boost/program_options#32 with boost 1.65+ we needed to change the behavior of the following flags to not accept `={true|false}` anymore:
        - `--use-locations-cache=false` becomes `--disable-location-cache`
        - `--parse-conditional-restrictions=true` becomes `--parse-conditional-restrictions`
        - The deprecated options `--use-level-cache` and `--generate-edge-lookup`
    - Bugfixes:
      - Fixed #4348: Some cases of sliproads pre-processing were broken
      - Fixed #4331: Correctly compute left/right modifiers of forks in case the fork is curved.
      - Fixed #4472: Correctly count the number of lanes using the delimter in `turn:lanes` tag.
      - Fixed #4214: Multiple runs of `osrm-partition` lead to crash.
      - Fixed #4348: Fix assorted problems around slip roads.
      - Fixed #4420: A bug that would result in unnecessary instructions, due to problems in suffix/prefix detection
    - Algorithm
      - Deprecate CoreCH functionality. Usage of CoreCH specific options will fall back to using CH with core_factor of 1.0
      - MLD uses a unidirectional Dijkstra for 1-to-N and N-to-1 matrices which yields speedup.

# 5.12.0
  - Changes from 5.11:
    - Guidance
      - now announcing turning onto oneways at the end of a road (e.g. onto dual carriageways)
      - Adds new instruction types at the exit of roundabouts and rotaries `exit roundabout` and `exit rotary`.
    - HTTP:
      - New query parameter for route/table/match/trip plugings:
        `exclude=` that can be used to exclude certain classes (e.g. exclude=motorway, exclude=toll).
        This is configurable in the profile.
    - NodeJS:
      - New query option `exclude` for the route/table/match/trip plugins. (e.g. `exclude: ["motorway", "toll"]`)
    - Profile:
      - New property for profile table: `excludable` that can be used to configure which classes are excludable at query time.
      - New optional property for profile table: `classes` that allows you to specify which classes you expect to be used.
        We recommend this for better error messages around classes, otherwise the possible class names are infered automatically.
    - Traffic:
      - If traffic data files contain an empty 4th column, they will update edge durations but not modify the edge weight.  This is useful for
        updating ETAs returned, without changing route selection (for example, in a distance-based profile with traffic data loaded).
    - Infrastructure:
      - New file `.osrm.cell_metrics` created by `osrm-customize`.
    - Debug tiles:
      - Added new properties `type` and `modifier` to `turns` layer, useful for viewing guidance calculated turn types on the map

# 5.11.0
  - Changes from 5.10:
    - Features
      - BREAKING: Added support for conditional via-way restrictions. This features changes the file format of osrm.restrictions and requires re-extraction
    - Internals
      - BREAKING: Traffic signals will no longer be represented as turns internally. This requires re-processing of data but enables via-way turn restrictions across highway=traffic_signals
      - Additional checks for empty segments when loading traffic data files
      - Tunes the constants for turns in sharp curves just a tiny bit to circumvent a mix-up in fork directions at a specific intersection (https://github.com/Project-OSRM/osrm-backend/issues/4331)
    - Infrastructure
      - Refactor datafacade to make implementing additional DataFacades simpler
    - Bugfixes
      - API docs are now buildable again
      - Suppress unnecessary extra turn instruction when exiting a motorway via a motorway_link onto a primary road (https://github.com/Project-OSRM/osrm-backend/issues/4348 scenario 4)
      - Suppress unnecessary extra turn instruction when taking a tertiary_link road from a teritary onto a residential road (https://github.com/Project-OSRM/osrm-backend/issues/4348 scenario 2)
      - Various MSVC++ build environment fixes
      - Avoid a bug that crashes GCC6
      - Re-include .npmignore to slim down published modules
      - Fix a pre-processing bug where incorrect directions could be issued when two turns would have similar instructions and we tried to give them distinct values (https://github.com/Project-OSRM/osrm-backend/pull/4375)
      - The entry bearing for correct the cardinality of a direction value (https://github.com/Project-OSRM/osrm-backend/pull/4353
      - Change timezones in West Africa to the WAT zone so they're recognized on the Windows platform

# 5.10.0
  - Changes from 5.9:
    - Profiles:
      - New version 2 profile API which cleans up a number of things and makes it easier to for profiles to include each other. Profiles using the old version 0 and 1 APIs are still supported.
      - New required `setup()` function that must return a configuration hash. Storing configuration in globals is deprecated.
      - Passes the config hash returned in `setup()` as an argument to `process_node/way/segment/turn`.
      - Properties are now set in `.properties` in the config hash returend by setup().
      - initialize raster sources in `setup()` instead of in a separate callback.
      - Renames the `sources` helper to `raster`.
      - Renames `way_functions` to `process_way` (same for node, segment and turn).
      - Removes `get_restrictions()`. Instead set `.restrictions` in the config hash in `setup()`.
      - Removes `get_name_suffix_list()`. Instead set `.suffix_list` in the config hash in `setup()`.
      - Renames `Handlers` to `WayHandlers`.
      - Pass functions instead of strings to `WayHandlers.run()`, so it's possible to mix in your own functions.
      - Reorders arguments to `WayHandlers` functions to match `process_way()`.
      - Profiles must return a hash of profile functions. This makes it easier for profiles to include each other.
      - Guidance: add support for throughabouts
    - Bugfixes
      - Properly save/retrieve datasource annotations for road segments ([#4346](https://github.com/Project-OSRM/osrm-backend/issues/4346)
      - Fix conditional restriction grammer parsing so it works for single-day-of-week restrictions ([#4357](https://github.com/Project-OSRM/osrm-backend/pull/4357))
    - Algorithm
      - BREAKING: the file format requires re-processing due to the changes on via-ways
      - Added support for via-way restrictions

# 5.9.2
    - API:
      - `annotations=durations,weights,speeds` values no longer include turn penalty values ([#4330](https://github.com/Project-OSRM/osrm-backend/issues/4330))

# 5.9.1
    - Infrastructure
      - STXXL is not required by default

# 5.9.0
  - Changes from 5.8:
    - Algorithm:
      - Multi-Level Dijkstra:
        - Plugins supported: `table`
        - Adds alternative routes support (see [#4047](https://github.com/Project-OSRM/osrm-backend/pull/4047) and [3905](https://github.com/Project-OSRM/osrm-backend/issues/3905)): provides reasonably looking alternative routes (many, if possible) with reasonable query times.
    - API:
      - Exposes `alternatives=Number` parameter overload in addition to the boolean flag.
      - Support for exits numbers and names. New member `exits` in `RouteStep`, based on `junction:ref` on ways
      - `Intersection` now has new parameter `classes` that can be set in the profile on each way.
    - Profiles:
      - `result.exits` allows you to set a way's exit numbers and names, see [`junction:ref`](http://wiki.openstreetmap.org/wiki/Proposed_features/junction_details)
      - `ExtractionWay` now as new property `forward_classes` and `backward_classes` that can set in the `way_function`.
         The maximum number of classes is 8.
      - We now respect the `construction` tag. If the `construction` tag value is not on our whitelist (`minor`, `widening`, `no`) we will exclude the road.
    - Node.js Bindings:
      - Exposes `alternatives=Number` parameter overload in addition to the boolean flag
      - Expose `EngineConfig` options in the node bindings
    - Tools:
      - Exposes engine limit on number of alternatives to generate `--max-alternatives` in `osrm-routed` (3 by default)
    - Infrastructure
      - STXXL is not required to build OSRM and is an optional dependency for back-compatibility (ENABLE_STXXL=On)
      - OpenMP is only required when the optional STXXL dependency is used
    - Bug fixes:
      - #4278: Remove superflous continious instruction on a motorway.

# 5.8.0
  - Changes from 5.7
    - API:
      - polyline6 support in request string
      - new parameter `approaches` for `route`, `table`, `trip` and `nearest` requests.  This parameter keep waypoints on the curb side.
        'approaches' accepts both 'curb' and 'unrestricted' values.
        Note : the curb side depend on the `ProfileProperties::left_hand_driving`, it's a global property set once by the profile. If you are working with a planet dataset, the api will be wrong in some countries, and right in others.
    - NodeJs Bindings
      - new parameter `approaches` for `route`, `table`, `trip` and `nearest` requests.
    - Tools
      - `osrm-partition` now ensures it is called before `osrm-contract` and removes inconsitent .hsgr files automatically.
    - Features
      - Added conditional restriction support with `parse-conditional-restrictions=true|false` to osrm-extract. This option saves conditional turn restrictions to the .restrictions file for parsing by contract later. Added `parse-conditionals-from-now=utc time stamp` and `--time-zone-file=/path/to/file`  to osrm-contract
      - Command-line tools (osrm-extract, osrm-contract, osrm-routed, etc) now return error codes and legible error messages for common problem scenarios, rather than ugly C++ crashes
      - Speed up pre-processing by only running the Lua `node_function` for nodes that have tags.  Cuts OSM file parsing time in half.
      - osrm-extract now performs generation of edge-expanded-edges using all available CPUs, which should make osrm-extract significantly faster on multi-CPU machines
    - Files
      - .osrm.nodes file was renamed to .nbg_nodes and .ebg_nodes was added
    - Guidance
      - #4075 Changed counting of exits on service roundabouts
    - Debug Tiles
      - added support for visualising turn penalties to the MLD plugin
      - added support for showing the rate (reciprocal of weight) on each edge when used
      - added support for turn weights in addition to turn durations in debug tiles
    - Bugfixes
      - Fixed a copy/paste issue assigning wrong directions in similar turns (left over right)
      - #4074: fixed a bug that would announce entering highway ramps as u-turns
      - #4122: osrm-routed/libosrm should throw exception when a dataset incompatible with the requested algorithm is loaded
      - Avoid collapsing u-turns into combined turn instructions

# 5.7.1
    - Bugfixes
      - #4030 Roundabout edge-case crashes post-processing

# 5.7.0
  - Changes from 5.6
    - Algorithm:
      - OSRM object has new option `algorithm` that allows the selection of a routing algorithm.
      - New experimental algorithm: Multi-Level Dijkstra with new toolchain:
        - Allows for fast metric updates in below a minute on continental sized networks (osrm-customize)
        - Plugins supported: `match` and `route`
        - Quickstart: `osrm-extract data.osm.pbf`, `osrm-partition data.osrm`, `osrm-customize data.osrm`, `osrm-routed --algorithm=MLD data.osrm`
    - NodeJs Bindings
      - Merged https://github.com/Project-OSRM/node-osrm into repository. Build via `cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_NODE_BINDINGS=On -DENABLE_MASON=On`.
      - `OSRM` object has new option `algorihtm="CH","CoreCH","MLD"`
    - Internals
      - Shared memory notification via conditional variables on Linux or semaphore queue on OS X and Windows with a limit of 128 OSRM Engine instances
    - Files
      - .osrm.datasource_index file was removed. Data is now part of .osrm.geometries.
      - .osrm.edge_lookup was removed. The option `--generate-edge-lookup` does nothing now.
      - `osrm-contract` does not depend on the `.osrm.fileIndex` file anymore
      - `osrm-extract` creates new file `.osrm.cnbg` and `.cnbg_to_ebg`
      - `osrm-partition` creates new file `.osrm.partition` and `.osrm.cells`
      - `osrm-customize` creates new file `.osrm.mldgr`
    - Profiles
      - Added `force_split_edges` flag to global properties. True value guarantees that segment_function will be called for all segments, but also could double memory consumption
    - Map Matching:
      - new option `gaps=split|ignore` to enable/disbale track splitting
      - new option `tidy=true|false` to simplify traces automatically

# 5.6.3
  - Changes from 5.6.0
    - Bugfixes
      - #3790 Fix incorrect speed values in tile plugin

# 5.6.2
  - Changes from 5.6.0
    - Bugfixes
      - Fix incorrect forward datasources getter in facade

# 5.6.1
  - Changes from 5.6.0
    - Bugfixes
      - Fix #3754 add restricted penalty on NoTurn turns

# 5.6.0
  - Changes from 5.5
    - Bugfixes
      - Fix #3475 removed an invalid `exit` field from the `arrive` maneuver
      - Fix #3515 adjusted number of `nodes` in `annotation`
      - Fix #3605 Fixed a bug that could lead to turns at the end of the road to be suppressed
      - Fix #2844 handle up to 16777215 code units in OSM names
    - Infrastructure
      - Support building rpm packages.
    - Guidance
      - No longer emitting turns on ferries, if a ferry should use multiple docking locations
    - Profiles
      - Removed the `./profile.lua -> ./profiles/car.lua` symlink. Use specific profiles from the `profiles` directory.
      - `properties` object has a new `weight_name` field, default value is "duration"
      - `properties` object has a new `weight_precision` field that specifies a decimal precision of edge weights, default value 1
      - In `way_function` the filed `forward_rate` and `backward_rate` of `ExtractionWay` can now be set.
        They have the same interpretation for the way weight as `forward_speed` and `backward_speed` for the edge duration.
        The unit of rate is meters per weight unit, so higher values will be prefered during routing.
      - `turn_function` now does not return an integer but takes in a `ExtractionTurn` object and can modify the `weight` and `duration` fields
      - `segment_function` now takes in a `ExtractionSegment` object and can modify `weight` and `duration` fields
      - `properties.uturn_penalty` is deprecated. Set it in the `turn_function`. The turn type is exposed as `ExtractionTurn::direction_modifier`.
      - `properties.traffic_light_penalty` is deprecated. Traffic light penalties now need to be set over in the turn function.
         Each turn with a traffic light is marked with `ExtractionTurn::has_traffic_light = true`.
      - Renamed the helper file `profiles/lib/directional.lua` to `profiles/lib/tags.lua` since it now provides more general tags parsing utility functions.
      - The car and foot profiles now depend on the helper file `profiles/lib/handlers.lua`.
    - Infrastructure
      - Disabled link-time optimized (LTO) builds by default. Enable by passing `-DENABLE_LTO=ON` to `cmake` if you need the performance and know what you are doing.
      - Datafile versioning is now based on OSRM semver values, rather than source code checksums.
        Datafiles are compatible between patch levels, but incompatible between minor version or higher bumps.
      - libOSRM now creates an own watcher thread then used in shared memory mode to listen for data updates
    - Tools:
      - Added osrm-extract-conditionals tool for checking conditional values in OSM data
    - Trip Plugin
      - Added a new feature that finds the optimal route given a list of waypoints, a source and a destination. This does not return a roundtrip and instead returns a one way optimal route from the fixed source to the destination points.

# 5.5.1
  - Changes from 5.5.0
    - API:
      - Adds `generate_hints=true` (`true` by default) which lets user disable `Hint` generating in the response. Use if you don't need `Hint`s!
    - Bugfixes
      - Fix #3418 and ensure we only return bearings in the range 0-359 in API responses
      - Fixed a bug that could lead to emitting false instructions for staying on a roundabout

# 5.5.0
  - Changes from 5.4.0
    - API:
      - `osrm-datastore` now accepts the parameter `--max-wait` that specifies how long it waits before aquiring a shared memory lock by force
      - Shared memory now allows for multiple clients (multiple instances of libosrm on the same segment)
      - Polyline geometries can now be requested with precision 5 as well as with precision 6
    - Profiles
      - the car profile has been refactored into smaller functions
      - get_value_by_key() is now guaranteed never to return empty strings, nil is returned instead.
      - debug.lua was added to make it easier to test/develop profile code.
      - `car.lua` now depends on lib/set.lua and lib/sequence.lua
      - `restrictions` is now used for namespaced restrictions and restriction exceptions (e.g. `restriction:motorcar=` as well as `except=motorcar`)
      - replaced lhs/rhs profiles by using test defined profiles
      - Handle `oneway=alternating` (routed over with penalty) separately from `oneway=reversible` (not routed over due to time dependence)
      - Handle `destination:forward`, `destination:backward`, `destination:ref:forward`, `destination:ref:backward` tags
      - Properly handle destinations on `oneway=-1` roads
    - Guidance
      - Notifications are now exposed more prominently, announcing turns onto a ferry/pushing your bike more prominently
      - Improved turn angle calculation, detecting offsets due to lanes / minor variations due to inaccuracies
      - Corrected the bearings returned for intermediate steps - requires reprocessing
      - Improved turn locations for collapsed turns
      - Sliproad classification refinements: the situations we detect as Sliproads now resemble more closely the reality
    - Trip Plugin
      - changed internal behaviour to prefer the smallest lexicographic result over the largest one
    - Bugfixes
      - fixed a bug where polyline decoding on a defective polyline could end up in out-of-bound access on a vector
      - fixed compile errors in tile unit-test framework
      - fixed a bug that could result in inconsistent behaviour when collapsing instructions
      - fixed a bug that could result in crashes when leaving a ferry directly onto a motorway ramp
      - fixed a bug in the tile plugin that resulted in discovering invalid edges for connections
      - improved error messages when missing files during traffic updates (#3114)
      - For single coordinate geometries the GeoJSON `Point` encoding was broken. We now always emit `LineString`s even in the one-coordinate-case (backwards compatible) (#3425)
    - Debug Tiles
      - Added support for turn penalties
    - Internals
      - Internal/Shared memory datafacades now share common memory layout and data loading code
      - File reading now has much better error handling
    - Misc
      - Progress indicators now print newlines when stdout is not a TTY
      - Prettier API documentation now generated via `npm run build-api-docs` output `build/docs`

# 5.4.3
  - Changes from 5.4.2
    - Bugfixes
      - #3254 Fixed a bug that could end up hiding roundabout instructions
      - #3260 fixed a bug that provided the wrong location in the arrival instruction

# 5.4.2
  - Changes from 5.4.1
    - Bugfixes
      - #3032 Fixed a bug that could result in emitting `invalid` as an instruction type on sliproads with mode changes
      - #3085 Fixed an outdated assertion that could throw without a cause for concern
      - #3179 Fixed a bug that could trigger an assertion in TurnInstruciton generation

# 5.4.1
  - Changes from 5.4.0
    - Bugfixes
      - #3016: Fixes shared memory updates while queries are running

# 5.4.0
  - Changes from 5.3.0
    - Profiles
      - includes library guidance.lua that offers preliminary configuration on guidance.
      - added left_hand_driving flag in global profile properties
      - modified turn penalty function for car profile - better fit to real data
      - return `ref` and `name` as separate fields. Do no use ref or destination as fallback for name value
      - the default profile for car now ignores HOV only roads
    - Guidance
      - Handle Access tags for lanes, only considering valid lanes in lane-guidance (think car | car | bike | car)
      - Improved the detection of non-noticeable name-changes
      - Summaries have been improved to consider references as well
    - API:
      - `annotations=true` now returns the data source id for each segment as `datasources`
      - Reduced semantic of merge to refer only to merges from a lane onto a motorway-like road
      - new `ref` field in the `RouteStep` object. It contains the reference code or name of a way. Previously merged into the `name` property like `name (ref)` and are now separate fields.
    - Bugfixes
      - Fixed an issue that would result in segfaults for viaroutes with an invalid intermediate segment when u-turns were allowed at the via-location
      - Invalid only_* restrictions could result in loss of connectivity. As a fallback, we assume all turns allowed when the restriction is not valid
      - Fixed a bug that could result in an infinite loop when finding information about an upcoming intersection
      - Fixed a bug that led to not discovering if a road simply looses a considered prefix
      - BREAKING: Fixed a bug that could crash postprocessing of instructions on invalid roundabout taggings. This change requires reprocessing datasets with osrm-extract and osrm-contract
      - Fixed an issue that could emit `invalid` as instruction when ending on a sliproad after a traffic-light
      - Fixed an issue that would detect turning circles as sliproads
      - Fixed a bug where post-processing instructions (e.g. left + left -> uturn) could result in false pronunciations
      - Fixes a bug where a bearing range of zero would cause exhaustive graph traversals
      - Fixes a bug where certain looped geometries could cause an infinite loop during extraction
      - Fixed a bug where some roads could be falsly identified as sliproads
      - Fixed a bug where roundabout intersections could result in breaking assertions when immediately exited
    - Infrastructure:
      - Adds a feature to limit results in nearest service with a default of 100 in `osrm-routed`

# 5.3.0
  - Changes from 5.3.0-rc.3
    - Guidance
      - Only announce `use lane` on required turns (not using all lanes to go straight)
      - Moved `lanes` to the intersection objects. This is BREAKING in relation to other Release Candidates but not with respect to other releases.
    - Bugfixes
      - Fix BREAKING: bug that could result in failure to load 'osrm.icd' files. This breaks the dataformat
      - Fix: bug that results in segfaults when `use lane` instructions are suppressed

  - Changes form 5.2.7
    - API
      - Introduces new `TurnType` in the form of `use lane`. The type indicates that you have to stick to a lane without turning
      - Introduces `lanes` to the `Intersection` object. The lane data contains both the markings at the intersection and a flag indicating if they can be chosen for the next turn
      - Removed unused `-s` from `osrm-datastore`
    - Guidance
      - Only announce `use lane` on required turns (not using all lanes to go straight)
      - Improved detection of obvious turns
      - Improved turn lane detection
      - Reduce the number of end-of-road instructions in obvious cases
    - Profile:
      - bicycle.lua: Surface speeds never increase the actual speed
    - Infrastructure
      - Add 32bit support
      - Add ARM NEON/VFP support
      - Fix Windows builds
      - Optimize speed file updates using mmap
      - Add option to disable LTO for older compilers
      - BREAKING: The new turn type changes the turn-type order. This breaks the **data format**.
      - BREAKING: Turn lane data introduces two new files (osrm.tld,osrm.tls). This breaks the fileformat for older versions.
    - Bugfixes:
      - Fix devide by zero on updating speed data using osrm-contract

# 5.3.0 RC3
  - Changes from 5.3.0-rc.2
    - Guidance
      - Improved detection of obvious turns
      - Improved turn lane detection
    - Bugfixes
      - Fix bug that didn't chose minimal weights on overlapping edges

# 5.3.0 RC2
  - Changes from 5.3.0-rc.1
    - Bugfixes
      - Fixes invalid checks in the lane-extraction part of the car profile

# 5.3.0 RC1
    - API
     - Introduces new `TurnType` in the form of `use lane`. The type indicates that you have to stick to a lane without turning
     - Introduces lanes to the route response. The lane data contains both the markings at the intersection and a flag indicating their involvement in the turn

    - Infrastructure
     - BREAKING: The new turn type changes the turn-type order. This breaks the **data format**.
     - BREAKING: Turn lane data introduces two new files (osrm.tld,osrm.tls). This breaks the fileformat for older versions.

# 5.2.5
  - Bugfixes
    - Fixes a segfault caused by incorrect trimming logic for very short steps.

# 5.2.4
  - Bugfixes:
    - Fixed in issue that arised on roundabouts in combination with intermediate intersections and sliproads

# 5.2.3
  - Bugfixes:
    - Fixed an issue with name changes in roundabouts that could result in crashes

# 5.2.2
  Changes from 5.2.1
  - Bugfixes:
    - Buffer overrun in tile plugin response handling

# 5.2.1
  Changes from 5.2.0
  - Bugfixes:
    - Removed debug statement that was spamming the console

# 5.2.0
  Changes from 5.2.0 RC2
   - Bugfixes:
     - Fixed crash when loading shared memory caused by invalid OSM IDs segment size.
     - Various small instructions handling fixes

   Changes from 5.1.0
   - API:
     - new parameter `annotations` for `route`, `trip` and `match` requests.  Returns additional data about each
       coordinate along the selected/matched route line per `RouteLeg`:
         - duration of each segment
         - distance of each segment
         - OSM node ids of all segment endpoints
     - Introducing Intersections for Route Steps. This changes the API format in multiple ways.
         - `bearing_before`/`bearing_after` of `StepManeuver` are now deprecated and will be removed in the next major release
         - `location` of `StepManeuvers` is now deprecated and will be removed in the next major release
         - every `RouteStep` now has property `intersections` containing a list of `Intersection` objects.
     - Support for destination signs. New member `destinations` in `RouteStep`, based on `destination` and `destination:ref`
     - Support for name pronunciations. New member `pronunciation` in `RouteStep`, based on `name:pronunciation`

   - Profile changes:
     - duration parser now accepts P[n]DT[n]H[n]M[n]S, P[n]W, PTHHMMSS and PTHH:MM:SS ISO8601 formats.
     - `result.destinations` allows you to set a way's destinations
     - `result.pronunciation` allows you to set way name pronunciations
     - `highway=motorway_link` no longer implies `oneway` as per the OSM Wiki

   - Infrastructure:
     - BREAKING: Changed the on-disk encoding of the StaticRTree to reduce ramIndex file size. This breaks the **data format**
     - BREAKING: Intersection Classification adds a new file to the mix (osrm.icd). This breaks the fileformat for older versions.
     - Better support for osrm-routed binary upgrade on the fly [UNIX specific]:
       - Open sockets with SO_REUSEPORT to allow multiple osrm-routed processes serving requests from the same port.
       - Add SIGNAL_PARENT_WHEN_READY environment variable to enable osrm-routed signal its parent with USR1 when it's running and waiting for requests.
     - Disable http access logging via DISABLE_ACCESS_LOGGING environment variable.

   - Guidance:
     - BREAKING: modifies the file format with new internal identifiers
     - improved detection of turning streets, not reporting new-name in wrong situations
     - improved handling of sliproads (emit turns instead of 'take the ramp')
     - improved collapsing of instructions. Some 'new name' instructions will be suppressed if they are without alternative and the segment is short

   - Bugfixes
     - fixed broken summaries for very short routes

# 5.2.0 RC2
   Changes from 5.2.0 RC1

   - Guidance:
     - improved handling of sliproads (emit turns instead of 'take the ramp')
     - improved collapsing of instructions. Some 'new name' instructions will be suppressed if they are without alternative and the segment is short
     - BREAKING: modifies the file format with new internal identifiers

   - API:
     - paramater `annotate` was renamed to `annotations`.
     - `annotation` as accidentally placed in `Route` instead of `RouteLeg`
     - Support for destination signs. New member `destinations` in `RouteStep`, based on `destination` and `destination:ref`
     - Support for name pronunciations. New member `pronunciation` in `RouteStep`, based on `name:pronunciation`
     - Add `nodes` property to `annotation` in `RouteLeg` containing the ids of nodes covered by the route

   - Profile changes:
     - `result.destinations` allows you to set a way's destinations
     - `result.pronunciation` allows you to set way name pronunciations
     - `highway=motorway_link` no longer implies `oneway` as per the OSM Wiki

   - Infrastructure
     - BREAKING: Changed the on-disk encoding of the StaticRTree to reduce ramIndex file size. This breaks the **data format**

   - Bugfixes
     - fixed broken summaries for very short routes

# 5.2.0 RC1
   Changes from 5.1.0

   - API:
     - new parameter `annotate` for `route` and `match` requests.  Returns additional data about each
       coordinate along the selected/matched route line.
     - Introducing Intersections for Route Steps. This changes the API format in multiple ways.
         - `bearing_before`/`bearing_after` of `StepManeuver` are now deprecated and will be removed in the next major release
         - `location` of `StepManeuvers` is now deprecated and will be removed in the next major release
         - every `RouteStep` now has property `intersections` containing a list of `Intersection` objects.

   - Profile changes:
     - duration parser now accepts P[n]DT[n]H[n]M[n]S, P[n]W, PTHHMMSS and PTHH:MM:SS ISO8601 formats.

   - Infrastructure:
     - Better support for osrm-routed binary upgrade on the fly [UNIX specific]:
       - Open sockets with SO_REUSEPORT to allow multiple osrm-routed processes serving requests from the same port.
       - Add SIGNAL_PARENT_WHEN_READY environment variable to enable osrm-routed signal its parent with USR1 when it's running and waiting for requests.
     - BREAKING: Intersection Classification adds a new file to the mix (osrm.icd). This breaks the fileformat for older versions.
     - Disable http access logging via DISABLE_ACCESS_LOGGING environment
       variable.

   - Guidance:
     - improved detection of turning streets, not reporting new-name in wrong situations

# 5.1.0
   Changes with regard to 5.0.0

   - API:
     - added StepManeuver type `roundabout turn`. The type indicates a small roundabout that is treated as an intersection
        (turn right at the roundabout for first exit, go straight at the roundabout...)
     - added StepManeuver type `on ramp` and `off ramp` to distinguish between ramps that enter and exit a highway.
     - reduced new name instructions for trivial changes
     - combined multiple turns into a single instruction at segregated roads`

   - Profile Changes:
    - introduced a suffix_list / get_name_suffix_list to specify name suffices to be suppressed in name change announcements
    - street names are now consistently assembled for the car, bike and walk profile as: "Name (Ref)" as in "Berlin (A5)"
    - new `car.lua` dependency `lib/destination.lua`
    - register a way's .nodes() function for use in the profile's way_function.

   - Infrastructure
    - BREAKING: reordered internal instruction types. This breaks the **data format**
    - BREAKING: Changed the on-disk encoding of the StaticRTree for better performance. This breaks the **data format**

   - Fixes:
    - Issue #2310: post-processing for local paths, fixes #2310
    - Issue #2309: local path looping, fixes #2309
    - Issue #2356: Make hint values optional
    - Issue #2349: Segmentation fault in some requests
    - Issue #2335: map matching was using shortest path with uturns disabled
    - Issue #2193: Fix syntax error position indicators in parameters queries
    - Fix search with u-turn
    - PhantomNode packing in MSVC now the same on other platforms
    - Summary is now not malformed when including unnamed roads
    - Emit new-name on when changing fron unanmed road to named road

# 5.0.0
   Changes with regard 5.0.0 RC2:
   - API:
     - if `geometry=geojson` is passed the resulting geometry can be a LineString or Point
       depending on how many coordinates are present.
     - the removal of the summary field was revered. for `steps=flase` the field will always be an empty string.

   Changes with regard to 4.9.1:
   - API:
     - BREAKING: Complete rewrite of the HTTP and library API. See detailed documentation in the wiki.
     - BREAKING: The default coordinate order is now `longitude, latidue`. Exception: Polyline geometry
         which follow the original Google specification of `latitdue, longitude`.
     - BREAKING: Polyline geometries now use precision 5, instead of previously 6
     - BREAKING: Removed GPX support
     - New service `tile` which serves debug vector tiles of the road network
     - Completely new engine for guidance generation:
        - Support for highway ramps
        - Support for different intersection types (end of street, forks, merges)
        - Instruction post-processing to merge unimportant instructions
        - Improved handling of roundabouts

   - Tools:
     - BREAKING: Renamed osrm-prepare to osrm-contract
     - BREAKING: Removes profiles from osrm-contract, only needed in osrm-extract.
     - Abort processing in osrm-extract if there are no snappable edges remaining.
     - Added .properties file to osrm-extract ouput.
     - Enables the use of multiple segment-speed-files on the osrm-contract command line

   - Profile changes:
     - Remove movable bridge mode
     - Add `maxspeed=none` tag to car profile.
     - A `side_road` tag support for the OSRM car profile.

   - Fixes:
     - Issue #2150: Prevents routing over delivery ways and nodes
     - Issue #1972: Provide uninstall target
     - Issue #2072: Disable alternatives by default and if core factor < 1.0
     - Issue #1999: Fix unpacking for self-loop nodes not in core.

   - Infrastructure:
     - Cucumber test suit is now based on cucumber-js, removes Ruby as dependency
     - Updated to mapbox/variant v1.1
     - Updated to libosmium v2.6.1
     - Remove GeoJSON based debugging output, replaced by debug tiles


# 5.0.0 RC2
   - Profiles:
      - `properties.allow_uturns_at_via` -> `properties.continue_straight_at_waypoint` (value is inverted!)
   - API:
      - Removed summary from legs property
      - Disable steps and alternatives by default
      - Fix `code` field: 'ok' -> 'Ok'
      - Allow 4.json and 4.3.json format
      - Conform to v5 spec and support "unlimited" as radiuses value.
      - `uturns` parameter was replaced by `continue_straight` (value is inverted!)
   - Features:
      - Report progress for gennerating edge expanded edges in the edge based graph factory
      - Add maxspeed=none tag to car profile.
      - Optimize StaticRTree code: speedup 2x (to RC1)
      - Optimize DouglasPeucker code: speedup 10x (to RC1)
      - Optimize WebMercator projection: speedup 2x (to RC1)
   - Bugs:
      - #2195: Resolves issues with multiple includedirs in pkg-config file
      - #2219: Internal server error when using the match plugin
      - #2027: basename -> filename
      - #2168: Report correct position where parsing failed
      - #2036: Add license to storage and storage config exposed in public API
      - Fix uturn detection in match plugin
      - Add missing -lz to fix linking of server-tests

# 5.0.0 RC1
   - Renamed osrm-prepare into osrm-contract
   - osrm-contract does not need a profile parameter anymore
   - New public HTTP API, find documentation [here](https://github.com/Project-OSRM/osrm-backend/wiki/New-Server-api)
   - POST support is discontinued, please use library bindings for more complex requests
   - Removed timestamp plugin
   - Coordinate order is now Longitude,Latitude
   - Cucumber tests now based on Javascript (run with `npm test`)
   - Profile API changed:
      - `forward_mode` and `backward_mode` now need to be selected from a pre-defined list
      - Global profile properties are now stored in a global `properties` element. This includes:
        - `properties.traffic_signal_penalty`
        - `properties.use_turn_restrictions`
        - `properties.u_turn_penalty`
        - `properties.allow_u_turn_at_via`
