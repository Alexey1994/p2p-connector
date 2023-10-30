process.env.NODE_TLS_REJECT_UNAUTHORIZED = '0'

const Imap = require('imap')
const fs = require('fs')
const { spawn } = require('child_process')

const credentials = JSON.parse(fs.readFileSync('../credentials.json', 'utf8'))

const imap = new Imap({
	user: credentials.email,
	password: credentials.password,
	host: 'imap.mail.ru',
	port: 993,
	tls: true
})

function getLastMessageHead() {
	return new Promise((resolve, reject) => {
		var f = imap.seq.fetch('*', {
			bodies: 'HEADER.FIELDS(FROM TO SUBJECT DATE)',
			struct: true
		})
		
		f.on('message', function(message, seqno) {
			message.on('body', function(stream, info) {
				var buffer = ''
				
				stream.on('data', function(chunk) {
					buffer += chunk.toString('utf8')
				})
				
				stream.once('end', function() {
					resolve(Imap.parseHeader(buffer))
				})
			})
		})
	})
}

function getLastMessageBody() {
	return new Promise((resolve, reject) => {
		var f = imap.seq.fetch('*', {
			bodies: 'TEXT',
			struct: true
		})
		
		f.on('message', function(message, seqno) {
			message.on('body', function(stream, info) {
				var buffer = ''
				
				stream.on('data', function(chunk) {
					buffer += chunk.toString('utf8')
				})
				
				stream.once('end', function() {
					resolve(buffer)
				})
			})
		})
	})
}

function serverConnect(clientAddress, onMyAddress) {
	const connect = spawn('server-connect.bat', [clientAddress])
	let receivedData = ''
	
	connect.stderr.on('data', data => {
		process.stdout.write(data.toString('utf8'))
	})
	
	connect.stdout.on('data', data => {
		receivedData += data.toString('utf8')
		
		if(receivedData.indexOf('\n') < 0) {
			return
		}
		
		const myAddress = receivedData.slice(0, receivedData.indexOf('\n'))
		
		onMyAddress(myAddress)
	})

	connect.on('close', code => {
		console.log('closed with code ' + code);
	})
}

imap.once('ready', () => {
	imap.subscribeBox('INBOX', true, (error, box) => {
		
	})
	
	imap.openBox('INBOX', true, (error, box) => {
		imap.on('mail', () => {
			getLastMessageHead()
				.then(mail => {
					if(mail.subject[0] !== 'connect') {
						return
					}
					
					getLastMessageBody()
						.then(body => {
							const clientAddress = body.slice(0, body.indexOf(' '))
							let sessionId = body.slice(body.indexOf(' ') + 1)
							
							while(sessionId.indexOf('\r') >= 0) {
								sessionId = sessionId.slice(0, sessionId.indexOf('\r'))
							}
							
							while(sessionId.indexOf('\n') >= 0) {
								sessionId = sessionId.slice(0, sessionId.indexOf('\n'))
							}
							
							serverConnect(clientAddress, myAddress => {
								console.log(`${sessionId}: ${myAddress} -> ${clientAddress}`)
								
								const sendAddress = spawn('send-address.bat', [mail.from[0], myAddress, sessionId])
								
								sendAddress.stdout.on('data', data => {
									
								})

								sendAddress.stderr.on('data', data => {
									
								})

								sendAddress.on('close', code => {
									//console.log('i am sended address')
								})
							})
						})
				})
		})
	})
})

imap.once('error', (error) => {
	
})

imap.once('end', () => {
	
})

imap.connect()