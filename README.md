# PostgreSQL client plugin

This plugin is a wrapper around [libpqxx](https://github.com/jtv/libpqxx).

## Installation

Follow the [instructions](https://docs.halon.io/manual/comp_install.html#installation) in our manual to add our package repository and then run the below command.

### Ubuntu

```
apt-get install halon-extras-postgresql
```

### RHEL

```
yum install halon-extras-postgresql
```

## Configuration

For the configuration schema, see [postgresql-app.schema.json](postgresql-app.schema.json).

### smtpd-app.yaml

```
plugins:
  - id: postgresql
    config:
      default_profile: postgresql
      profiles:
        - id: postgresql
          host: postgresql
          port: 5432
          username: postgres
          password: postgres
          connect_timeout: 5
          pool_size: 32
```

## Exported classes

These classes needs to be [imported](https://docs.halon.io/hsl/structures.html#import) from the `extras://postgresql` module path.

### PostgreSQL([string $profile])

The PostgreSQL class is a [libpqxx](https://github.com/jtv/libpqxx) wrapper class. On error an exception is thrown.

#### query(string $query [, array $params]): array

Pass a "prepared" statement to the PostgreSQL server. On error an exception is thrown. See the table below for the data types mapping.

| PostgreSQL  | HSL       |
|-------------|-----------|
| `bool`      | `boolean` |
| `int`       | `number`  |
| `int2`      | `number`  |
| `int4`      | `number`  |
| `serial2`   | `number`  |
| `serial4`   | `number`  |
| `float4`    | `number`  |
| `float8`    | `number`  |
| `*`         | `string`  |

## Examples

```
import { PostgreSQL } from "extras://postgresql";

// Create "PostgreSQL" instance with an (optional) config profile
$postgresql = PostgreSQL("postgresql");
$postgresql->query("CREATE TABLE IF NOT EXISTS employee (name VARCHAR(255), salary INT, working BOOLEAN, age SMALLINT)");
$postgresql->query("INSERT INTO employee (name, salary, working, age) VALUES ($1, $2, $3, $4)", ["John Doe", 54321, true, 36]);
$postgresql->query("INSERT INTO employee (name, salary, working, age) VALUES ($1, $2, $3, $4)", ["Jane Doe", 54321, false, 36]);
echo $postgresql->query("SELECT * FROM employee");
// [0=>["name"=>"John Doe","salary"=>54321,"working"=>true,"age"=>36],1=>["name"=>"Jane Doe","salary"=>54321,"working"=>false,"age"=>36]]
```
