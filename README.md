# RouteApplication
A C++ application that calculates and renders a route, using [OpenStreetMap](https://www.openstreetmap.org/) map data.

![Image of Athens, Greece](https://raw.githubusercontent.com/lazaros-katiniotis/RouteApplication/master/example.png)

## Overview
The application supports querying OpenStreetMap servers to access the map data directly or loading existing map data files. The library that is used to download data from the OSM servers is [CURL](https://curl.se/). [Pugixml](https://pugixml.org/) is used to parse the OSM data. Once the data are parsed they are stored in the Model class, which is a simple intepretation of the actual OSM data. After the Model is created, a path is calculated using the A* search algorithm on the nodes from all road data. Finally, the rendering library that is used to render the map data and the path is [io2d](https://github.com/cpp-io2d/P0267_RefImpl).

## Usage
Running the application with no other command line arguments provides the user with an example of the command line arguments used to download a map by providing a query of the bounding area and a starting and ending position for the calculation of the route.

The command line arguments are:

### bound
    -b min_lon min_lat max_lon max_lat
Initiates a HTTP request for a rectangular region covered by the coordinates *min_lon, min_lat, max_lon* and *max_lat*. 


### point
    -p lon lat
Initiates a HTTP request for a small rectangular region arround a point of the *lon* and *lat* coordinates.


### file
    -f filename1.xml
    -f filename2.osm
Loads an existing map data file and does not download data from the OSM API.


### bound and file
    -b min_lon min_lat max_lon max_lat -f filename.xml
    -b min_lon min_lat max_lon max_lat -f filename.osm
Initiates a HTTP request for a rectangular region covered by the coordinates *min_lon, min_lat, max_lon* and *max_lat*, and stores the data in the file *filename.xml* or *filename.osm*.


### point and file
    -p lon lat -f filename.xml
    -p lon lat -f filename.osm
Initiates a HTTP request for a small rectangular region arround a point of the *lon* and *lat* coordinates, and stores the data in the file *filename.xml* or *filename.osm*.


### start and end
    -start x y -end x y
Initializes the starting and ending point of the route. *x* and *y* are coordinates relative to the application window, with a range of values [0,1].


## Example
The following example downloads a bounding area of map data, initializes a starting and ending point for the route calculation, and stores the data downloaded in a file named *example.osm*.
```
./RouteApplication.exe -b 23.7255 37.9666 23.7315 37.9705 -start 0.25 0.25 -end 0.75 0.75 -f example.osm
```
![Image of Athens, Greece](https://raw.githubusercontent.com/lazaros-katiniotis/RouteApplication/master/example.png)


## License
Under MIT License.
