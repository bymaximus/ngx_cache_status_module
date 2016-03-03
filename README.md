# cache_status

Модуль собирает статистику по кэшированию запросов и предоставляет
доступ к этой статистике.

## Сборка

```
./configure ..... --add-module=PATH_TO_MODULE/ngx_cache_status_module
make
```

## Директивы

```
Синтаксис: cache_status;
Значение по умолчанию: ---
Контекст: location
```
Предоставляет статистику кэширования.

## Пример конфигурации

```
location /cache-stat {
    cache_status;
}
```

## Пример данных

```
Cache statistics:
Requests: 15
Uncached: 10
Miss: 2
Bypass: 0
Expired: 1
Stale: 0
Updating: 0
Revalidated: 0
Hit: 2
Misc: 0
```