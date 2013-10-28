//function defines from pccam
#ifdef __cplusplus
extern "C" {            //  Assume C declarations for C++
#endif  //C++

// Das hab ich mal reinkopiert 
typedef void *HANDLE;
typedef unsigned long       DWORD;

int INITBOARD(int board,HANDLE *hdriver);
// initializes board

//in:
// board= number of board
// *hdriver
// if(NULL) the driver is loaded and
//          the board initialized
//          *hdriver gives back the filehandle
//          of the driver for this board
// if(!NULL) the board is initialized
// 
// after initialization a short test is executed
// set camera mode, allocate memory, start camera
// grab one picture, stop camera, free memory 

int INITBOARDP(int board,HANDLE *hdriver);
// same as INITBOARD() without test at end


int CLOSEBOARD(HANDLE *hdriver);
// reset the board and closes the driver
// *hdriver = the opened driver
//            set to NULL  				

int RESETBOARD(HANDLE hdriver);
// reset the board 
// do not use this function if 
// any other action is done with the board 

//in:
// hdriver = filehandle returned from INITBOARD


int GETBOARDPAR(HANDLE hdriver,void *buf, int len);
//Returns len status bytes from board

//in:
// hdriver = filehandle returned from INITBOARD
// buf     = address of buffer
// len     = bytes to read  

//out:
// *buf    = boardparameters 

int SETMODE(HANDLE hdriver,int mode,
			               int explevel,int exptime, 
			               int hbin,int vbin,
						   int gain,int offset,
						   int bit_pix,int shift);
// set the parameter for the next exposures
// this function cannot be called if the 
// camera is running. All parameters are checked.

//in:
// hdriver = filehandle returned from INITBOARD

// mode :
//set mode of camera
//       = 0x10  single asnyc shutter hardware trigger  
//       = 0x11  single asnyc shutter software trigger  
//       = 0x40  single auto exposure hardware trigger  
//       = 0x41  single auto exposure software trigger  
//only one exposure is released by a HW-trigger or 
//SW-trigger.
//Timing: making exposure then readout CCD. After this 
//a new trigger is accepted.


// exptime  :
//Set exposure time of camera
//The possible exposurtime range is from 10 to 10000�s.

         
// explevel  :
//Set level in % which time to stop auto exposure mode
//The possible exposurelevel is from 0 to 255.
//the camera must be a lightmeter version
         

// hbin  :
//Set horizontal binning of the camera
//       = 0  horizontal x1
//       = 1  horizontal x2 

// vbin  :
//Set vertical binning of the camera
//       = 0  vertical x1
//       = 1  vertical x2 


// gain :
//set gain value of camera (HW version 03)
//       = 0  low gain
//       = 1  high gain 


// offset  :
//Set analog offset level of camera
//Not valid in with newer boards, always set to zero


// bitpix :
// set how many bits per pixel are transferred

//       = 12  12bits per pixel, shift =0
//             2 bytes with the upper 4 bits
//             set to zero are sent. Therefore 
//             two pixel values are moved with one 
//             PCI(32 bit) transfer. 

//       =  8  8bits per pixel,shift possible
//             8 bit values are generated with a
//             programmable barrel shifter from the 
//             12 bit A/D values. Therefore four 
//             pixel are moved with one PCI transfer.
//             This half's the pixel data per image
//             and frees the PCI bus.



// shift
// set the digital gain value
// only valid in 8 Bitperpixel mode

//       = 0	8 bit (D11..D4), digital gain x1
//       = 1 	8 bit (D10..D3), digital gain x2
//       = 2 	8 bit (D9..D2), digital gain x4
//       = 3 	8 bit (D8..D1), digital gain x8
//       = 4 	8 bit (D7..D0), digital gain x16
//       = 5 	8 bit (D6..D0), digital gain x32


int WRRDORION(HANDLE hdriver,int cmnd,int *data);
//writes an comand to the ORION-controller
//and reads back the data value send.

//in:
// hdriver = filehandle returned from INITBOARD
// cmnd    = comand to send

//out:
// *data   = the data send back, by the ORION-controller

//implemented comands in ORION1.14:
//	10h	rd_portA
//  11h	rd_portB
//  13h	rd_portD
//  20h	wr_portC


int SET_EXPOSURE(HANDLE hdriver,int time);
//set exposure time in async mode without stopping camera
//(updated HW necessary) 

//in:
// hdriver = filehandle returned from INITBOARD
// time    = new exposuretime


int TRIGGER_CAMERA(HANDLE hdriver);
//This releases a single exposure in SW-Trigger mode.

//in:
// hdriver = filehandle returned from INITBOARD

int START_CAMERA(HANDLE hdriver);
int STOP_CAMERA(HANDLE hdriver);
//These commands start and stop the mode setting. 
//All settings of variables, like binning or gain
//etc. should be done when the camera mode is stopped
//with STOP_CAMERA. When this command returns without
//error then the CCD is cleared and ready for a new
//exposure. This can be released with START_CAMERA 
//and a trigger command. 

//in:
// hdriver = filehandle returned from INITBOARD

int GETSIZES(HANDLE hdriver,int *ccdxsize,int *ccdysize,
			                int *actualxsize,int *actualysize,
							int *bit_pix);
//Return the CCD-Size and the actual size in Pixel 
//in:
// hdriver = filehandle returned from INITBOARD

//out:
// *ccdxsize     = x-resolution of CCD
// *ccdysize     = y-resolution of CCD
// *actualxsize  = x-resolution of picture
// *actualysize  = y-resolution of picture
// *bit_pix      = bits per pixel in picture
//                 (12 or 8)

int READTEMPERATURE(HANDLE hdriver,int *ccd);
//Return actual CCD-temperature
//The range is from -55�C to +125�C  

//in:
// hdriver = filehandle returned from INITBOARD

//out:
// *ccd    = temperature in �C.  


int READVERSION(HANDLE hdriver,int typ,char *vers,int len);
//Return version strings from the camera
//in:
// hdriver = filehandle returned from INITBOARD
// typ     = typ of version string
// len     = lenght of buffer vers

//out:
// *vers   = string  




int GETBUFFER_STATUS(HANDLE hdriver,int bufnr,int mode,int *stat,int len);
//Returns len status bytes from buffer

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer
// mode    = set internal to 0   
// len     = bytes to read  

//out:
// first dword *stat   = status of buffer
        //dmawrite               0x00000001 
        //dmawrite done          0x00000002
        //queued                 0x00000004
        //canceled with stop     0x00000008

        //event set              0x00000010
        //event write done       0x00000020
        //mapped                 0x00000080

        //burst error in dma     0x00001000
        //size error in dma      0x00002000
        //event error            0x00004000
        //timeout in dma         0x00008000

        //dmawrite prepared      0x10000000
        //not vxd allocated      0x40xx0000
        //buffer locked          0x20xx0000
        //vxd allocated          0x80xx0000
        //xx=buffernumber 0-128

// *(stat+1) = last exptime in �s


int ADD_BUFFER_TO_LIST(HANDLE hdriver,int bufnr,int size,int offset,int data);
//Set a buffer into the buffer queue
//the driver holds a list of 32 buffers
//if the buffer is the first in the list a picture
//transfer is started immediatly if the camera runs
//the next buffer are stored in the list. If a transfer
//is done and there is a buffer in the list the next tranfer
//is started immediatly.   

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer from ALLOCATE_BUFFER
// size    = number of bytes to transfer
//           for 12bit data actualxsize*actualysize*2  
//           for 8bit data actualxsize*actualysize  
//           lower values are possible
//           greater values will timeout the dma
//           or come back with error 
// offset  = offset in the buffer
// data    = 0 not implemented yet

int REMOVE_BUFFER_FROM_LIST(HANDLE hdriver,int bufnr);
//Clear the buffer int the buffer queue 
//If a transfer is actual in progress to this buffer
//a error is returned.

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer from ALLOCATE_BUFFER


int ALLOCATE_BUFFER(HANDLE hdriver,int *bufnr,int *size);
//Allocate a buffer for the camera in PC-Mainmemory

//in:
// hdriver = filehandle returned from INITBOARD
// *size   = size of buffer in byte
// *bufnr  = -1 for new buffer
//           number of allocated buffer to 
//           reallocate with other size

//out:
// *size   = allocated size, which might be greater
// *bufnr  = number of buffer
//          


int FREE_BUFFER(HANDLE hdriver,int bufnr);
//Free allocated buffer

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer

int SETBUFFER_EVENT(HANDLE hdriver,int bufnr,HANDLE *hPicEvent);
//Create a eventhandle for this buffer.
//The event is set when the DMA-Transfer into the
//buffer is finished or if a error occurred in transfer
//Use i.e WaitForSingleObject(*hPicEvent,TimeOut); to
//wait until a trnsfer is done

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer

//out:
// *hPicEvent = handle of event

int MAP_BUFFER(HANDLE hdriver,int bufnr,int size,int offset,DWORD *linadr);
//Map the buffer to a user address 

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer
// size    = bytes of buffer to map
// offset  = 0

//out:
// *linadr = address of buffer

int UNMAP_BUFFER(HANDLE hdriver,int bufnr);
//Unmap the buffer  
//Please unmap all mapped buffers before
//closing the filehandle

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer




//The ORION prozessor is called automaticly by the driver
//shortly after a dmatransfer is done.
//With the following functions one can set the comands
//and data-byte, which belongs to the every comand.
//One can sent up to 16 comands. If the driver find a
//comand in the comad table, it will catch the data_in byte
//from the same tableposition and send this to the 
//ORION-prozessor. After the ORION has finished the comand
//and has written back his data-byte, this byte will be stored
//in the data_back table at the same tableposition, from where the
//comand is read out.
//If the comand has the value 0x00 or position 16 is reached,
//the driver will stop sending comands   
//Each buffer has its own table, so you can define different comands
//for each buffer.
// 


int SETORIONINT(HANDLE hdriver,int bufnr, int mode,unsigned char *cmnd,int len);
//Write len bytes to the comand or data table for the driver internal ORION call

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer
// mode    = 1 orion data_back
//           2 orion data_in
//           3 orion comand          
// cmnd    = address of buffer of comands 
//           or data to set, maximal 16 bytes
// len     = length of buffer

int GETORIONINT(HANDLE hdriver,int bufnr, int mode,unsigned char *data,int len);
//Read len bytes from the comand or data tables for the driver internal ORION call. 

//in:
// hdriver = filehandle returned from INITBOARD
// bufnr   = number of buffer
// mode    = 1 orion data_back
//           2 orion data_in
//           3 orion comand          
// cmnd    = address of buffer 
// len     = length of buffer

//out:
// *cmnd   = Comands or data read

int READEEPROM(HANDLE hdriver,int mode,int adr,char *data);
//Read one byte from the Eeprom at address adr.
//Do not call, if camera is running

//in:
// hdriver = filehandle returned from INITBOARD
// mode    = 0 HEAD-EEPROM
//           1 CARD-EEPROM
// adr     = address of byte to read ( 0-255 ) 

//out:
// *data   = Byte read


int WRITEEEPROM(HANDLE hdriver,int mode,int adr,char data);
//Read one byte from the Eeprom at address adr. 
//Do not call, if camera is running

//in:
// hdriver = filehandle returned from INITBOARD
// mode    = 0 HEAD-EEPROM
//           1 CARD-EEPROM
// adr     = address of byte to read ( 0-127 ) 
// data    = Byte to write


int SETTIMEOUTS(HANDLE hdriver,DWORD dma, DWORD proc, DWORD head);
//Can set timout values for cardio,dma and head  

//in:
// hdriver = filehandle returned from INITBOARD
// dma     = timeout in ms for dma
// proc    = timeout in ms for cardio
// head    = timeout in ms for headpoll

int SETDRIVER_EVENT(HANDLE hdriver,int mode,HANDLE *hHeadEvent);
//Create a eventhandle for driver Events.
//The event is set when the Head connects or disconnects.
//Use i.e WaitForSingleObject(*hHeadEvent,TimeOut); to
//wait and react on this events

//in:
// hdriver = filehandle returned from INITBOARD
// mode    = low word    0x0000=Headevent
//           high word   0x0000=open and enable event 
//                       0x8000=disable event
//                       0xC000=disable and close event

//out:
// *hHeadEvent = handle of event

//@ver1.008

//for the following commands special HW is required

int READ_TEMP(HANDLE hdriver,int *ccd_temp,int *ele_temp);
//Return actual CCD-temperature and actual elektronic temperature
//The range is from -55�C to +125�C  

//in:
// hdriver = filehandle returned from INITBOARD

//out:
// *ccd_temp    = temperature of CCD in �C.  
// *ele_temp    = temperature of electronic in �C.  


int SET_NOMINAL_PELTIER_TEMP(HANDLE hdriver,int temp);
//Set nominal temperature of peltier cooling
//The range is from -10�C to +25�C  

//in:
// hdriver = filehandle returned from INITBOARD
// temp     = nominal temperature of peltier in �C.  

int GET_NOMINAL_PELTIER_TEMP(HANDLE hdriver,int *temp);
//Set nominal temperature of peltier cooling

//in:
// hdriver = filehandle returned from INITBOARD

//out:
// *temp     = nominal temperature of peltier in �C.  


int SET_STANDBY_MODE(HANDLE hdriver,int mode);
//Set standy mode for card and head

//in:
// hdriver = filehandle returned from INITBOARD
// mode    = Combination of the following Bit defines  off=0, on=1 
//           Bit0:  Fan off/on   
//           Bit1:  Peltier off/on   
//           Bit2:  Head Elektronic off/on   

int GET_STANDBY_MODE(HANDLE hdriver,int *mode);
//get standy mode for card and head

//in:
// hdriver = filehandle returned from INITBOARD

//out:
// *mode    = Combination of the following Bit defines  off=0, on=1 
//           Bit0:  Fan off/on   
//           Bit1:  Peltier off/on   
//           Bit2:  Head Elektronic off/on   


#ifdef __cplusplus
}
#endif  //C++

//
