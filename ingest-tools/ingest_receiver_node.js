/////////////////////////////////////////// 
//        media ingest receiver          //
//         DASH-IF CMAF ingest           //
///////////////////////////////////////////
 
const http = require('http')
var fs = require('fs');
var url = require('url');

var boxes = ["ftyp","moov","moof","styp","emsg","prft"];

var active_streams = new Map()

const server = http.createServer(function(request, response) {
  console.dir(request.param)

  if (request.method == 'POST') 
  {
	
	var url_parts = url.parse(request.url)
    var path = url_parts.pathname.split("/").join("_")
    var body = ''
    
	// 
    request.on('data', function(data) {
      
	  if(body == '')
	  {
		  console.log('begin of request')  
	  }
	  //else
	  //{
		  //console.log('continue request')
	  //}
	  
	  body += data 
    });
	
	// end the node.js javascript
    request.on('end', function() 
	{
	var box = ""
	var is_frag = new Boolean(false); 
	var is_init = new Boolean(false);
	
	if(body.length > 8)
	{
	  box = body.slice(4,8)
	 
	  is_frag = new Boolean((box == boxes[2]) ||(box == boxes[3]) || (box == boxes[4]) || (box == boxes[5]) || (box == boxes[6]))
	  is_init = new Boolean((box == boxes[0]) || (box == boxes[1]) )
	  console.log(box)
	}
    
	
	// not yet initialized, initialized
	if( !active_streams.has(path) && is_init) // initialize with init fragment
	{
		active_streams.set(path,"initialized") 
	    // check that the body is a valid fmp4 fragment
	    fs.appendFile(path, body, function (err) 
	    {
           //if (err) throw err;
           // console.log('Saved!');
        }); 
        response.writeHead(200, {'Content-Type': 'text/html'})
        response.end('init fragment received OK')
	}
	else if(active_streams.has(path) && is_frag )
	{
	  // check that the body is a valid fmp4 fragment
	  fs.appendFile(path, body, function (err) 
	  {
         //if (err) throw err;
         // console.log('Saved!');
      }); 
      response.writeHead(200, {'Content-Type': 'text/html'})
      response.end('media fragment received OK')
	}
	else // request the init fragment
	{
		if(!active_streams.has(path) && is_frag ) // fragment for non initialized stream
		{
		   response.writeHead(412, {'Content-Type': 'text/html'})
           response.end('need init fragment or CMAF Header')  
		}
		else if(active_streams.has(path) && is_init) // duplicate init fragment
		{
			response.writeHead(200, {'Content-Type': 'text/html'})
            response.end('duplicate CMAF Header or init fragment received')	
		}
	}   
    
	
    });

   }  
		// write the error message
	request.on('error', (err) => {
       // This prints the error message and stack trace to `stderr`.
       console.error(err.stack);
	   response.writeHead(400, {'Content-Type': 'text/html'})
       response.end('error in request')
	   
    }) 
})

const port = 80
const host = '127.0.0.1'
server.listen(port, host)
console.log(`Listening at http://${host}:${port}`)