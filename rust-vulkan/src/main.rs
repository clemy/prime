// Prime number search using Vulcan GPU shader
// Copyright (C) 2019 Bernhard C. Schrenk <clemy@clemy.org>

// This code is based on the vulkano example from http://vulkano.rs
//   Copyright (c) 2017 The vulkano developers
//   Licensed under the Apache License, Version 2.0

use std::time::Instant;
use std::sync::Arc;
use vulkano::buffer::BufferUsage;
use vulkano::buffer::CpuAccessibleBuffer;
use vulkano::command_buffer::AutoCommandBufferBuilder;
use vulkano::command_buffer::CommandBuffer;
use vulkano::descriptor::descriptor_set::PersistentDescriptorSet;
use vulkano::device::Device;
use vulkano::device::DeviceExtensions;
use vulkano::device::Features;
use vulkano::instance::Instance;
use vulkano::instance::InstanceExtensions;
use vulkano::instance::PhysicalDevice;
use vulkano::pipeline::ComputePipeline;
use vulkano::sync::GpuFuture;

fn main() {
    let start = Instant::now();

    let instance = Instance::new(None, &InstanceExtensions::none(), None)
        .expect("failed to create instance");

    let physical = PhysicalDevice::enumerate(&instance).next().expect("no device available");

    let queue_family = physical.queue_families()
        .find(|&q| q.supports_compute())
        .expect("couldn't find a compute queue family");

    let (device, mut queues) = {
        Device::new(physical, &Features::none(), &DeviceExtensions::none(),
                    [(queue_family, 0.5)].iter().cloned()).expect("failed to create device")
    };

    let queue = queues.next().unwrap();

    // Introduction to compute operations
    let data_iter = 0u32 .. 327680;
    let data_buffer = CpuAccessibleBuffer::from_iter(device.clone(), BufferUsage::all(),
                                                    data_iter).expect("failed to create buffer");

    // Compute pipelines
    mod cs {
        vulkano_shaders::shader!{
            ty: "compute",
            src: "
#version 450

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0) buffer Data {
    uint data[];
} buf;

bool checkPrime(uint p) {
    uint max = uint(sqrt(p) + 1);
    for (uint i = 3; i <= max; i += 2) {
        if (p % i == 0) {
            return false;
        }
    }
    return true;
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    uint tocheck = 1000001 + idx * 1280; // kStart + idx * kCountPerThread
    for (uint i = 0; i < 1280 / 64; ++i) { // kCountPerThread / 64
        uint bitset = 0;
        for (uint j = 0; j < 32; ++j) {
            if (checkPrime(tocheck)) {
                bitset |= 1 << j;
            }
            tocheck += 2;
        }    
        buf.data[idx * (1280 / 64) + i] = bitset; // kCountPerThread / 16
    }
}"
        }
    }

    let shader = cs::Shader::load(device.clone())
        .expect("failed to create shader module");
    let compute_pipeline = Arc::new(ComputePipeline::new(device.clone(), &shader.main_entry_point(), &())
        .expect("failed to create compute pipeline"));

    // Descriptor sets
    let set = Arc::new(PersistentDescriptorSet::start(compute_pipeline.clone(), 0)
        .add_buffer(data_buffer.clone()).unwrap()
        .build().unwrap()
    );

    // Dispatch
    let command_buffer = AutoCommandBufferBuilder::new(device.clone(), queue.family()).unwrap()
        .dispatch([256, 1, 1], compute_pipeline.clone(), set.clone(), ()).unwrap()
        .build().unwrap();

    let finished = command_buffer.execute(queue.clone()).unwrap();
    finished.then_signal_fence_and_flush().unwrap()
        .wait(None).unwrap();

    let duration1 = start.elapsed();

    let content = data_buffer.read().unwrap();
    for (i, val) in content.iter().enumerate() {
        for j in 0..32 {
            if *val & (1u32 << j) != 0 {
                println!("{}", 1000001 + (i * 32 + j) * 2);
            }
        }
    }
    let duration2 = start.elapsed();

    eprintln!("Calculation-Time: {:?}", duration1);
    eprintln!("Complete-Time: {:?}", duration2);
}
