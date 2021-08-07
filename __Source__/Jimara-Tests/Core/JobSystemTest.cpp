#include "../GtestHeaders.h"
#include "Core/Systems/JobSystem.h"
#include "OS/Logging/StreamLogger.h"


namespace Jimara {
	namespace {
		template<typename Type>
		class Value : public virtual JobSystem::Job {
		public:
			virtual Type Get()const = 0;
		};

		class SimpleCounter : public virtual Value<uint64_t> {
		private:
			std::atomic<uint64_t> m_count = 0;

		protected:
			inline virtual void Execute()override { m_count++; }
			inline virtual void CollectDependencies(Callback<Job*>) override {}

		public:
			inline virtual uint64_t Get()const override { return m_count; }
		};
	}

	// Tests execution without dependencies or threads
	TEST(JobSystemTest, IndependentExecutionSingleThreaded) {
		JobSystem system(1);

		ASSERT_TRUE(system.Execute());

		Reference<SimpleCounter> counterA = Object::Instantiate<SimpleCounter>();
		ASSERT_EQ(counterA->Get(), 0);

		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 0);

		system.Add(counterA);
		ASSERT_EQ(counterA->Get(), 0);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 1);

		system.Remove(counterA);
		ASSERT_EQ(counterA->Get(), 1);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 1);

		Reference<SimpleCounter> counterB = Object::Instantiate<SimpleCounter>();
		ASSERT_EQ(counterB->Get(), 0);

		system.Add(counterB);
		ASSERT_EQ(counterA->Get(), 1);
		ASSERT_EQ(counterB->Get(), 0);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 1);
		ASSERT_EQ(counterB->Get(), 1);

		system.Add(counterA);
		ASSERT_EQ(counterA->Get(), 1);
		ASSERT_EQ(counterB->Get(), 1);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 2);
		ASSERT_EQ(counterB->Get(), 2);

		std::vector<Reference<SimpleCounter>> counters;
		for (size_t i = 0; i < 1024; i++) {
			counters.push_back(Object::Instantiate<SimpleCounter>());
			for (size_t j = 0; j < counters.size(); j++)
				ASSERT_EQ(counters[j]->Get(), counters.size() - j - 1);
			system.Add(counters.back());
			system.Execute();
		}
	}

	// Tests multithreaded execution without dependencies
	TEST(JobSystemTest, IndependentExecutionMultithreaded) {
		JobSystem system(std::thread::hardware_concurrency());
		std::vector<Reference<SimpleCounter>> counters;
		for (size_t i = 0; i < 1024; i++) {
			counters.push_back(Object::Instantiate<SimpleCounter>());
			for (size_t j = 0; j < counters.size(); j++)
				ASSERT_EQ(counters[j]->Get(), counters.size() - j - 1);
			system.Add(counters.back());
			system.Execute();
		}
	}


	namespace {
		class SimpleSum : public virtual Value<uint64_t> {
		private:
			std::atomic<uint64_t> m_value = 0;
			std::vector<Reference<Value<uint64_t>>> m_values;

		protected:
			inline virtual void Execute()override {
				m_value = 0;
				for (size_t i = 0; i < m_values.size(); i++)
					m_value += m_values[i]->Get();
			}

			inline virtual void CollectDependencies(Callback<Job*> record) override {
				for (size_t i = 0; i < m_values.size(); i++)
					record(m_values[i]);
			}

		public:

			inline void AddDependency(Value<uint64_t>* value) { m_values.push_back(value); }

			template<typename... Values>
			inline void AddDependency(Value<uint64_t>* first, Value<uint64_t>* second, Values... rest) {
				AddDependency(first);
				AddDependency(second, rest...);
			}

			inline void RemoveDependency(Value<uint64_t>* value) {
				for (size_t i = 0; i < m_values.size(); i++)
					if (m_values[i] == value) {
						std::swap(m_values[i], m_values.back());
						m_values.pop_back();
						break;
					}
			}

			inline SimpleSum() {}

			template<typename... Values>
			inline SimpleSum(Value<uint64_t>* first, Values... rest) { AddDependency(first, rest...); }

			inline virtual uint64_t Get()const override { return m_value; }
		};
	}


	// Checks basic dependencies
	TEST(JobSystemTest, DependentExecutionSingleThreaded) {
		JobSystem system(1);

		Reference<SimpleSum> simpleSum = Object::Instantiate<SimpleSum>();
		ASSERT_EQ(simpleSum->Get(), 0);
		system.Add(simpleSum);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(simpleSum->Get(), 0);

		Reference<SimpleCounter> counterA = Object::Instantiate<SimpleCounter>();

		simpleSum->AddDependency(counterA);
		ASSERT_EQ(counterA->Get(), 0);
		ASSERT_EQ(simpleSum->Get(), 0);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 1);
		ASSERT_EQ(simpleSum->Get(), 1);

		simpleSum->RemoveDependency(counterA);
		ASSERT_EQ(counterA->Get(), 1);
		ASSERT_EQ(simpleSum->Get(), 1);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 1);
		ASSERT_EQ(simpleSum->Get(), 0);

		Reference<SimpleCounter> counterB = Object::Instantiate<SimpleCounter>();

		simpleSum->AddDependency(counterA);
		simpleSum->AddDependency(counterB);
		ASSERT_EQ(counterA->Get(), 1);
		ASSERT_EQ(counterB->Get(), 0);
		ASSERT_EQ(simpleSum->Get(), 0);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 2);
		ASSERT_EQ(counterB->Get(), 1);
		ASSERT_EQ(simpleSum->Get(), 3);

		system.Remove(simpleSum);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 2);
		ASSERT_EQ(counterB->Get(), 1);
		ASSERT_EQ(simpleSum->Get(), 3);

		system.Add(counterA);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 3);
		ASSERT_EQ(counterB->Get(), 1);
		ASSERT_EQ(simpleSum->Get(), 3);

		system.Add(simpleSum);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 4);
		ASSERT_EQ(counterB->Get(), 2);
		ASSERT_EQ(simpleSum->Get(), 6);

		system.Remove(counterA);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 5);
		ASSERT_EQ(counterB->Get(), 3);
		ASSERT_EQ(simpleSum->Get(), 8);

		system.Remove(simpleSum);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 5);
		ASSERT_EQ(counterB->Get(), 3);
		ASSERT_EQ(simpleSum->Get(), 8);

		Reference<SimpleSum> sumOfAll = Object::Instantiate<SimpleSum>(counterA, counterB, counterA, simpleSum);

		system.Add(sumOfAll);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(counterA->Get(), 6);
		ASSERT_EQ(counterB->Get(), 4);
		ASSERT_EQ(simpleSum->Get(), 10);
		ASSERT_EQ(sumOfAll->Get(), 26);
	}

	namespace {
		class SimpleValue : public virtual Value<uint64_t> {
		private:
			std::atomic<uint64_t> m_value = 0;

		protected:
			inline virtual void Execute()override {}
			inline virtual void CollectDependencies(Callback<Job*>) override {}

		public:
			inline SimpleValue(uint64_t value) : m_value(value) {}
			inline virtual uint64_t Get()const override { return m_value; }
		};

		Reference<Value<uint64_t>> CreateFastBinomialSum(size_t rows) {
			std::vector<Reference<Value<uint64_t>>> rowBuffers[2];
			std::vector<Reference<Value<uint64_t>>>* curRow = rowBuffers;
			std::vector<Reference<Value<uint64_t>>>* prevRow = rowBuffers + 1;
			for (size_t row = 0; row <= rows; row++) {
				std::swap(curRow, prevRow);
				curRow->clear();
				for (size_t i = 0; i <= row; i++)
					curRow->push_back(((i <= 0) || (i >= row))
						? Reference<JobSystem::Job>(Object::Instantiate<SimpleValue>(1))
						: Reference<JobSystem::Job>(Object::Instantiate<SimpleSum>((*prevRow)[i - 1], (*prevRow)[i])));
			}
			Reference<SimpleSum> sum = Object::Instantiate<SimpleSum>();
			for (size_t i = 0; i < curRow->size(); i++)
				sum->AddDependency(curRow->operator[](i));
			return sum;
		}

		static const size_t DEFAULT_BINOMIAL_ROW_COUNT = 30;
	}

	// Calculates fast binomial sum for high level dependence
	TEST(JobSystemTest, FastBinomialSumSingleThreaded) {
		JobSystem system(1);
		Reference<Value<uint64_t>> binomial = CreateFastBinomialSum(DEFAULT_BINOMIAL_ROW_COUNT);
		system.Add(binomial);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(binomial->Get(), ((uint64_t)1u) << DEFAULT_BINOMIAL_ROW_COUNT);
	}

	// Calculates fast binomial sum with multiple threads
	TEST(JobSystemTest, FastBinomialSumMultithreaded) {
		JobSystem system(std::thread::hardware_concurrency());
		Reference<Value<uint64_t>> binomial = CreateFastBinomialSum(DEFAULT_BINOMIAL_ROW_COUNT);
		system.Add(binomial);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(binomial->Get(), ((uint64_t)1u) << DEFAULT_BINOMIAL_ROW_COUNT);
	}

	namespace {
		class SlowBinomialValue : public virtual Value<uint64_t> {
		private:
			const uint64_t m_row;
			const uint64_t m_column;
			std::atomic<uint64_t> m_value = 0;

			static uint64_t Value(uint64_t row, uint64_t column) {
				if ((row <= 0) || (column <= 0) || (column >= row)) return 1;
				else return Value(row - 1, column - 1) + Value(row - 1, column);
			}

		protected:
			inline virtual void Execute()override { m_value = Value(m_row, m_column); }
			inline virtual void CollectDependencies(Callback<Job*>) override {}

		public:
			inline SlowBinomialValue(uint64_t row, uint64_t column) : m_row(row), m_column(column) {}
			inline virtual uint64_t Get()const override { return m_value; }
		};

		Reference<Value<uint64_t>> CreateSlowBinomialSum(size_t rows) {
			Reference<SimpleSum> sum = Object::Instantiate<SimpleSum>();
			for (uint64_t i = 0; i <= rows; i++)
				sum->AddDependency(Object::Instantiate<SlowBinomialValue>(rows, i));
			return sum;
		}
	}

	// Counts binomial sum the slow way with single thread
	TEST(JobSystemTest, SlowBinomialSumSingleThreaded) {
		JobSystem system(1);
		Reference<Value<uint64_t>> binomial = CreateSlowBinomialSum(DEFAULT_BINOMIAL_ROW_COUNT);
		system.Add(binomial);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(binomial->Get(), ((uint64_t)1u) << DEFAULT_BINOMIAL_ROW_COUNT);
	}

	// Counts binomial sum the slow way with multiple threads to illustrate speedup
	TEST(JobSystemTest, SlowBinomialSumMultithreaded) {
		JobSystem system(std::thread::hardware_concurrency());
		Reference<Value<uint64_t>> binomial = CreateSlowBinomialSum(DEFAULT_BINOMIAL_ROW_COUNT);
		system.Add(binomial);
		ASSERT_TRUE(system.Execute());
		ASSERT_EQ(binomial->Get(), ((uint64_t)1u) << DEFAULT_BINOMIAL_ROW_COUNT);
	}


	// Tests circular dependencies
	TEST(JobSystemTest, Errors) {
		JobSystem system(std::thread::hardware_concurrency());
		Reference<OS::StreamLogger> logger = Object::Instantiate<OS::StreamLogger>();

		Reference<SimpleSum> sumA = Object::Instantiate<SimpleSum>();
		Reference<SimpleSum> sumB = Object::Instantiate<SimpleSum>();
		Reference<SimpleSum> sumC = Object::Instantiate<SimpleSum>();
		
		system.Add(sumA);
		ASSERT_TRUE(system.Execute(logger));
		
		// A->B
		sumA->AddDependency(sumB);
		ASSERT_TRUE(system.Execute(logger));
		
		// A->B->C
		sumB->AddDependency(sumC);
		ASSERT_TRUE(system.Execute(logger));
		
		// A->B->C->A
		sumC->AddDependency(sumA);
		ASSERT_FALSE(system.Execute(logger));
		
		// A->B; B->C; A->C
		sumA->AddDependency(sumC);
		sumC->RemoveDependency(sumA);
		ASSERT_TRUE(system.Execute(logger));

		// A->B; C->A; A->C;
		sumC->AddDependency(sumA);
		ASSERT_FALSE(system.Execute(logger));

		// A->B; A->C;
		sumC->RemoveDependency(sumA);
		ASSERT_TRUE(system.Execute(logger));

		// A->C;
		sumA->RemoveDependency(sumB);
		ASSERT_TRUE(system.Execute(logger));

		// A->C; C->A;
		sumC->AddDependency(sumA);
		ASSERT_FALSE(system.Execute(logger));

		// A->C;
		sumC->RemoveDependency(sumA);
		ASSERT_TRUE(system.Execute(logger));

		// NO DEPENDENCIES
		sumA->RemoveDependency(sumC);
		ASSERT_TRUE(system.Execute(logger));

		// A->A
		sumA->AddDependency(sumA);
		ASSERT_FALSE(system.Execute(logger));
	}
}
