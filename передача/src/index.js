process.env.NODE_TLS_REJECT_UNAUTHORIZED = '0'

const nodemailer = require('nodemailer')
const fs = require('fs')

const credentials = JSON.parse(fs.readFileSync('../credentials.json', 'utf8'))
const mailTo = process.argv[2]
const address = process.argv[3]
const sessionId = process.argv[4]

const transport = nodemailer.createTransport({
	host: 'smtp.mail.ru',
	port: 465,
	secure: true,
	auth: {
		user: credentials.email,
		pass: credentials.password
	}
})

transport.sendMail({
	from: credentials.email,
	to: [mailTo],
	subject: 'connect',
	text: `${address} ${sessionId}`
})

transport.close()