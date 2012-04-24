Overview
--------
The repository provides a Leaflet [(1)] based web frontend to the Open Source Routing Machine (Project-OSRM [(2)]).
The frontend is implemented in Javascript.
Data is fetched from routing and geocoding servers using JSONP queries.
The website is XHTML 1.0 Strict compliant.
A deployed version of the the web frontend can be seen at [(3)].


Setup
-----
The frontend should work directly as provided.
Several settings - including the URL for the routing server and the geocoder server - can be specified in `OSRM.config.js`.
Different tile servers can be specified in `OSRM.Map.js`.
Note that the URL shortener used for generating route links only works with URLs pointing to the official Project-OSRM website.


Branches
--------
* The `master` branch will always point to the latest released version of the frontend.
* The `develop` branch should always point to a working version with new features and bugfixes (think of it as a nightly-build).
* Other branches contain various work in progress.


Bugtracking
-----------
Please use the OSRM-Project bug tracker [(4)] for submitting any bug reports or feature requests.


Contribute
----------
If you like to contribute, you can simply fork the project and start coding.
When you are going to provide a more substantial addition, please create a new branch first.
For pull requests use the develop branch as target, never the master branch. 


Integration into Project-OSRM repository
----------------------------------------
The Project-OSRM repository already contains the frontend repository as a submodule.
It will always point to the latest deployed version.
To successfully work a repository that contains submodules, use the following git commands (available in git 1.7.1+):

* `git clone --recursive`  
	to clone a repository and the contained submodules

* `git pull && git submodule update`  
	to pull the latest version of the repository and update its submodules if required

Note that the frontend can also be checked out independently of the Project-OSRM repository.


References
----------
[(1)] Cloudmade Leaflet: http://leaflet.cloudmade.com/  
[(2)] Project OSRM: http://project-osrm.org/  
[(3)] Project OSRM Frontend: http://map.project-osrm.org/  
[(4)] Project OSRM Bugtracker: https://github.com/DennisOSRM/Project-OSRM/issues/


[(1)]: http://leaflet.cloudmade.com/ "Cloudmade Leaflet"
[(2)]: http://project-osrm.org/ "Project OSRM"
[(3)]: http://map.project-osrm.org/ "Project-OSRM Frontend" 
[(4)]: https://github.com/DennisOSRM/Project-OSRM/issues/ "Project-OSRM Bugtracker"