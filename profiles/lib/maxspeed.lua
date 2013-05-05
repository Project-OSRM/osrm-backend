local math = math

module "MaxSpeed"

function limit(way,max,maxf,maxb)
    if maxf and maxf>0 then
        way.forward.speed = math.min(way.forward.speed, maxf)
	elseif max and max>0 then
		way.forward.speed = math.min(way.forward.speed, max)
	end
	
    if maxb and maxb>0 then
        way.backward.speed = math.min(way.backward.speed, maxb)
	elseif max and max>0 then
		way.backward.speed = math.min(way.backward.speed, max)
	end
end
