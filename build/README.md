# Build instructions

```
export HALON_REPO_USER=exampleuser
export HALON_REPO_PASS=examplepass
docker compose -p halon-extras-postgresql up --build
docker compose -p halon-extras-postgresql down --rmi local
```
