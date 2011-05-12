<?
    $url = $_SERVER['HTTP_HOST'];
    if($url == "map.project-osrm.org") {
    	$file = file_get_contents('map.html');
		echo $file;
    	return;
   }
   $file = file_get_contents('main.html');
	echo $file;
	return;
?>