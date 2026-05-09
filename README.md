# inferno

a key logger 'agent' with logs fetched by a remote 'server' app

test your service :
Ex with agent service

```
docker compose up server --no-log-prefix 2>&1 | cat
```
```
docker compose up agent --scale agent=3
```