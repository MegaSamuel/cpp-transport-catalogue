# Транспортный справочник TransportCatalogue

## Описание
Транспортный справочник TransportCatalogue представляет собой автобусный маршрутизатор с графической визуализацией

## Использование
Работа транспортного справочника разделена на два этапа:

- на вход поступает JSON-файл c запросами на построение базы данных. В этом файле содержится информация об остановках, автобусных марштрутах, настройках визуализации. Производится сериализация базы данных;
- автобусный каталог десериализуется из сохраненной базы данных. Затем обрабатываются несколько видов запросов: 
- - получение информации об остановке или автобусе;
- - вычисление самого быстрого маршрута между заданными остановками;
- - визуализация карты.

## Сборка
Сборка производится из командной строки с использованием утилиты CMake.
Рядом с кататогом transport-catalogue создать каталог build и перейти в него.
Выполнить команды:
- cmake ../transport-catalogue -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=c:/path/to/protobuf/
- cmake --build .

В каталоге build/Release/ будет создан исполняемый файл transport_catalogue

## Системные требования
Компилятор GCC с поддержкой стандарта C++17 или выше.
Установленная утилита CMake версии не ниже 3.10.
Установленный пакет бинарной сериализации данных Protobuf версии не ниже 3.0