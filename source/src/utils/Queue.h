#ifndef __TOM_QUEUE__
#define __TOM_QUEUE__

	template <class T>

	class Queue{
		private:
			Node<T>* first;
			Node<T>* last;			
			int count;
		
		public:
			Queue(){
				this->first = NULL;
				this->last = NULL;
				this->count = 0;
			}
		
			~Queue(){
				while (this->notEmpty()){
					this->dequeue();
				}
			}
		
			bool notEmpty(){				
				return count>0;
			}

			void enqueue(T data){
				Node<T>* element = new Node<T>();
				element->setData(data);
				element->setNext(NULL);
				
				if (this->notEmpty()){	
					this->last->setNext(element);
					this->last = element;
				}
				else{
					this->first = element;
					this->last = element;			
				}
				this->count++;
			}
			
			bool dequeue(){
				if (this->notEmpty()){				
					Node<T>* tmp = this->first;
					this->first = tmp->getNext();				
					this->count--;
					delete tmp;
					return true;
				}
				return false;
			}
		
			bool peak(T* data){			
				if (this->notEmpty()){
					*data = this->first->getData();				
					return true;
				}
				return false;
			}
	};

#endif