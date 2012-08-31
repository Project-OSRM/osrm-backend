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
Settings - including URLs of the routing server, geocoder server and tile servers - are specified in `OSRM.config.js`.
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
If you like to contribute, simply fork the project and start coding.
It is best practice to create a new branch (from the current master) with a descriptive name for your contributions.
When you are done, send a pull request from that branch.
With this workflow, each pull request is isolated and can be easily merged.


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


Compatibility
-------------
The frontend has been tested with Firefox 3.0+, Internet Explorer 8+ and Chrome 18+.
Certain visuals like rounded corners or moving boxes will only show in newer browser versions.
But no actual functionality is affected by this.
Note that the frontend will not work with Internet Explorer 6 or 7.


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