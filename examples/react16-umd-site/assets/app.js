(function () {
  'use strict';
  function Counter(props) {
    React.Component.call(this, props);
    this.state = { count: 0 };
    this.increment = this.increment.bind(this);
  }
  Counter.prototype = Object.create(React.Component.prototype);
  Counter.prototype.constructor = Counter;
  Counter.prototype.increment = function () {
    this.setState({ count: this.state.count + 1 });
  };
  Counter.prototype.render = function () {
    return React.createElement('main', { id: 'react-app' },
      React.createElement('h1', null, 'React 16 under Venom'),
      React.createElement('p', { id: 'count' }, String(this.state.count)),
      React.createElement('button', { id: 'increment', onClick: this.increment }, 'Increment')
    );
  };
  ReactDOM.render(React.createElement(Counter), document.getElementById('root'));
}());
