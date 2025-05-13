How to find the power surge in tracks
open moboxterm and connect to rasberry pi with host id 192.168.1.30
make sure marvel mind is on and is sening its current location. open dashboard to check this.
then type the commands :
make clean && make
./Surveillance_arret
Surveillance_arret.c contains the program that gets the marvelmind location data of x and y coordinates and send it to canbus i.e can0
open browser and type http://192.168.1.30:1880/ to access nodered of the train. there is seperate flow for each train select the flow accordingly,
 you can check in debug that required canframes with can id 32 i.e volatage canframe and can id 291 which is the location canframe.
open virtual box in the system and login to alpine315_docker.
open browser and type http://192.168.1.109:1880/  to access the nodered for the database. 
http://192.168.1.109:8080/?server=192.168.1.109&username=root&db=infra&select=Trames3 go to this link to access the database and select the train number accordingly in which you are using.
select the train number and you can see the sql database with canid 32,291,48. below the screen you can see the download database option then download the entire database with data in it. 
you can clear the clearbase and see the fresh data getting added into the database.
open terminal, add this csv file in the data and then run the python file powersurge.py to get the output of powersurge points,its location and balise number.
open the powerpredict.py file and name csv files accordingly then run it to see the power predicted points in the output txt file with location and balise number of the points.
to determine the track surges open the tracksurge.py file and give input accordingly and run the code to see the tracksurges on the track.
