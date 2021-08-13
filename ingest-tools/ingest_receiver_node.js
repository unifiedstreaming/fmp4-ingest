/////////////////////////////////////////// 
//        media ingest receiver          //
//         DASH-IF CMAF ingest           //
//     not completed, input welcome      //
//                                       //
//    Only short running pots and        //
//  usage of Streams() keyword mandatory //
//                                       //
///////////////////////////////////////////
 
const http = require('http')
var fs = require('fs');
var url = require('url');

var boxes = ["ftyp","moov","moof","styp","emsg","prft"];

var active_streams = new Map()

const server = http.createServer(function(request, response) {
  console.dir(request.param)

  request.setEncoding('Binary')
  let chunks = []
  
  if (request.method == 'POST') 
  {
     var url_parts = url.parse(request.url)
     var path  = url_parts.pathname.split("/").join('')
     //var index = path.search("Streams(");
     var fn = path 
    
     fn=path.substring(8, path.length -1)
     fn= "out_js_".concat(fn)

   
     var body  = new Buffer('');
    
	// 
     request.on('data', function(chunk) 
	 {
	  if(body == '')
	  {
		  console.log('begin of request')  
	  }
	  chunks.push(Buffer.from(chunk, 'binary'));
     });
	
	// end the node.js javascript
     request.on('end', function() 
	{
	var box = ""
	var is_frag = new Boolean(false); 
	var is_init = new Boolean(false);
	let binary = Buffer.concat(chunks);
	
	if(binary.length > 8)
	{
	  box = binary.slice(4,8)
	 
	  is_frag = new Boolean((box == boxes[2]) ||(box == boxes[3]) || (box == boxes[4]) || (box == boxes[5]) || (box == boxes[6]))
	  is_init = new Boolean((box == boxes[0]) || (box == boxes[1]) )
	  
	}
    
	
	// not yet initialized, initialized
	if( !active_streams.has(path) && is_init) // initialize with init fragment
	{
		active_streams.set(path,"initialized") 
	    // check that the body is a valid fmp4 fragment
		// var buf = new Buffer(body, 'base64')
		console.log("|| Appending CMAF Header ! ")
	    fs.appendFile(fn, binary, null, function (err) 
	    {
           //if (err) throw err;
           // console.log('Saved!');
        }); 
        response.writeHead(200, {'Content-Type': 'text/html'})
        response.end('init fragment received OK')
	}
	else if(active_streams.has(path) && is_frag )
	{
	  console.log("appending media segment!")
	  // check that the body is a valid fmp4 fragment
	  fs.appendFile(fn, binary, null, function (err) 
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
           response.end(' Need init segment or CMAF Header')  
		}
		else if(active_streams.has(path) && is_init) // duplicate init fragment not supported yet
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

const port = 8080
const host = '127.0.0.1'
server.listen(port, host)
console.log(`Listening at http://${host}:${port}`)
