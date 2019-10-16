# ELK Stack

Specify your log location in docker-compose.yml->logstash->volume mapping
```
// change "/Users/xunliu/Desktop/git/elastic-example/data/" to your data location
// later will abstract this to parameters
/Users/xunliu/Desktop/git/elastic-example/data/:/data
```

Start
```
docker-compose -f docker-compose.yml up
```

Stop and delete all temp data
```
docker-compose -f docker-compose.yml down -v
```

You could go to following link for Kibana
```
http://localhost:5601
```

If you need one component specific logs, you could use
```
docker-compose logs | grep logstash
```

If you need debug specific service, for example, debug logstash
```
// find your logstash's container id
docker container ls

// enter container
docker exec -it a03643622486 /bin/bash

// go to logstash folder
./bin/logstash --config.test_and_exit -f /usr/share/logstash/config/log
```
