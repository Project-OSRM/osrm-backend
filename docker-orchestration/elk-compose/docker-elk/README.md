# ELK Stack

Specify your log location in .env file and change DATA_PATH there
```
// change "/Users/xunliu/Desktop/git/elastic-example/data/" to your data location
DATA_PATH=/Users/xunliu/Desktop/go/src/github.com/Telenav/logs/
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
// username: elastic
// password: changeme
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
