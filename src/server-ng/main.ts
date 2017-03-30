import * as express from 'express';
import * as morgan from 'morgan';
import * as cors from 'cors';


// Services take Requests and assemble Responses
// https://expressjs.com/en/api.html#req
// https://expressjs.com/en/api.html#res
interface ServiceHandler {
  Handle(req: express.Request, res: express.Response, next: express.NextFunction): void;
}


class RouteHandler implements ServiceHandler {
  public Handle(req: express.Request, res: express.Response, next: express.NextFunction) {
    res.json({'status': 'Ok', 'message': 'route'});
  }
}

class TableHandler implements ServiceHandler {
  public Handle(req: express.Request, res: express.Response, next: express.NextFunction) {
    res.json({'status': 'Ok', 'message': 'table'});
  }
}


class HttpServer {
  constructor(host: string, port: number) {
    this.host = host;
    this.port = port;
    this.app = express();

    // Middlewares; refactor this, move out

    this.app.use(cors());

    this.app.use(morgan('tiny'));  // change to 'combined' for Apache-style log

    const router = express.Router();

    const routeHandler = new RouteHandler();
    const tableHandler = new TableHandler();

    router.get('/route/*', routeHandler.Handle);
    router.get('/table/*', tableHandler.Handle);

    this.app.use(router);
  }

  public Start() {
    this.app.listen(this.port, this.host, () => {
      console.log(`Listening on ${this.host}:${this.port}`);
    });
  }


  public host: string;
  public port: number;

  private app: express.Application;
}


function main() {
  const args = process.argv.slice(1);

  const host = args[1] || '127.0.0.1';
  const port = Number.parseInt(args[2]) || 5000;

  const server = new HttpServer(host, port);
  server.Start();
}


if (require.main === module) { main(); }
