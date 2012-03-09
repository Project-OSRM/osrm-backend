// compatibility mode for old browser
function getElementsByClassName(node, classname) {
    var a = [];
    var re = new RegExp('(^| )'+classname+'( |$)');
    var els = node.getElementsByTagName("*");
    for(var i=0,j=els.length; i<j; i++)
        if(re.test(els[i].className))a.push(els[i]);
    return a;
}
document.head = document.head || document.getElementsByTagName('head')[0];

// ------------------------------------------------------

// human readable time
function secondsToTime(seconds){
   seconds = parseInt(seconds);
   minutes = parseInt(seconds/60);
   seconds = seconds%60;
   hours = parseInt(minutes/60);
   minutes = minutes%60;
   if(hours==0){
   	return minutes+' min';
   }
   else{
   	return hours+' h '+minutes+' min';
   }
}

// human readable distance
function getDistanceWithUnit(distance){
	distance = parseInt(distance);
	if(distance >= 1000){ return (parseInt(distance/1000))+' km'; }
	else{ return distance+' m'; }
}

// ------------------------------------------------------

// distance between two points
function distanceBetweenPoint(x1, y1, x2, y2) {
	var a = x1 - x2;
	var b = y1 - y2;
	return Math.sqrt(a*a + b*b);
}

// distance from a point to a line or segment
// (x,y)   point
// (x0,y0) line point A
// (x1,y1) line point B
// o       specifies if the distance should respect the limits of the segment (overLine = true)
//         or if it should consider the segment as an infinite line (overLine = false);
//         if false returns the distance from the point to the line,
//         otherwise the distance from the point to the segment
function dotLineLength(x, y, x0, y0, x1, y1, o){
	function lineLength(x, y, x0, y0){return Math.sqrt((x -= x0) * x + (y -= y0) * y);}

	if(o && !(o = function(x, y, x0, y0, x1, y1){
		if(!(x1 - x0)) return {x: x0, y: y};
		else if(!(y1 - y0)) return {x: x, y: y0};
		var left, tg = -1 / ((y1 - y0) / (x1 - x0));
		return {x: left = (x1 * (x * tg - y + y0) + x0 * (x * - tg + y - y1)) / (tg * (x1 - x0) + y0 - y1), y: tg * left - tg * x + y};
    }(x, y, x0, y0, x1, y1) && o.x >= Math.min(x0, x1) && o.x <= Math.max(x0, x1) && o.y >= Math.min(y0, y1) && o.y <= Math.max(y0, y1))){
		var l1 = lineLength(x, y, x0, y0), l2 = lineLength(x, y, x1, y1);
		return l1 > l2 ? l2 : l1;
    }
    else {
    	var a = y0 - y1, b = x1 - x0, c = x0 * y1 - y0 * x1;
        return Math.abs(a * x + b * y + c) / Math.sqrt(a * a + b * b);
    }
};