process.env.NODE_TLS_REJECT_UNAUTHORIZED = '0'

const Imap = require('imap')
const fs = require('fs')

const credentials = JSON.parse(fs.readFileSync('../credentials.json', 'utf8'))
const mailFrom = process.argv[2]
const sessionId = process.argv[3]

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

imap.once('ready', () => {
	imap.subscribeBox('INBOX', true, (error, box) => {
		
	})
	
	imap.openBox('INBOX', true, (error, box) => {
		imap.on('mail', () => {
			getLastMessageHead()
				.then(mail => {
					if(mail.subject[0] !== 'connect' || mail.from[0] !== mailFrom) {
						return
					}
					
					getLastMessageBody()
						.then(body => {
							const address = body.slice(0, body.indexOf(' '))
							let mailSessionId = body.slice(body.indexOf(' ') + 1)
							
							while(mailSessionId.indexOf('\r') >= 0) {
								mailSessionId = mailSessionId.slice(0, mailSessionId.indexOf('\r'))
							}
							
							while(mailSessionId.indexOf('\n') >= 0) {
								mailSessionId = mailSessionId.slice(0, mailSessionId.indexOf('\n'))
							}
							
							if(mailSessionId !== sessionId) {
								return
							}
							
							console.log(address)
							
							imap.end()
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