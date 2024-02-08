export type Handler = (...args: any[]) => void;
export type Emitter = ReturnType<typeof createEmitter>;
export const createEmitter = () => ({
  _eventHandlers: {} as Record<string, Handler[] | undefined>,
  on(event: string, handler: Handler): () => void {
    this._eventHandlers[event]?.push(handler) ?? (this._eventHandlers[event] = [handler]);

    return () => {
      const handlerIndex = this._eventHandlers[event]?.indexOf(handler) ?? -1;

      if (handlerIndex > -1) {
        this._eventHandlers[event]?.splice(handlerIndex, 1);
      }
    };
  },
  emit(event: string, ...args: any[]): void {
    this._eventHandlers[event]?.forEach((handler: Handler) => handler(...args));
  },
});
